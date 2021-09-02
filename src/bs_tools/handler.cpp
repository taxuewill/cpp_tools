//
// Created by Will on 2021/8/28.
//

#include "bs_tools/handler.h"
#include <iostream>
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>

namespace bs_tools {

    using namespace std;
    using namespace chrono;

    //Message
    Message::Message() : mWhen(getCurrentTimeStamp()), mWhat(-1), mCallback(nullptr), mNext(nullptr) {}

    Message::~Message() {
#ifdef DEBUG
        cout << "destroy message:" << mWhat << endl;
#endif
    }

    SptrMessage Message::obtain(SptrHandler handler, int what) {
        SptrMessage m = std::make_shared<Message>();
        m->mWhat=what;
        m->mTarget = handler;
        return m;
    }
    SptrMessage Message::obtain() {
        return std::make_shared<Message>();
    }

    time_t getCurrentTimeStamp() {
        time_point<system_clock, milliseconds> tp = time_point_cast<milliseconds>(system_clock::now());
        auto tmp = duration_cast<milliseconds>(tp.time_since_epoch());
        time_t timestamp = tmp.count();
        return timestamp;
    }

    MessageQueue::MessageQueue() {
        mWakeEventFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        mEpollFd = epoll_create(8);
        if (mWakeEventFd < 0 || mEpollFd < 0) {
            cout << "Error fd < 0" << endl;
        }
        struct epoll_event eventItem;
        memset(&eventItem, 0, sizeof(epoll_event));
        eventItem.events = EPOLLIN;
        eventItem.data.fd = mWakeEventFd;
        int result = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mWakeEventFd, &eventItem);
        if (result != 0) {
            cout << "Could not add wake event fd to epoll instance: " << strerror(errno) << endl;
        }
#ifdef DEBUG
        cout<<"create MessageQueue"<<endl;
#endif

    }

    MessageQueue::~MessageQueue() {
        cout << "destroy MessageQueue" << endl;
        close(mWakeEventFd);
        if (mEpollFd >= 0) {
            close(mEpollFd);
        }
    }

    bool MessageQueue::enqueueMessage(std::shared_ptr<Message> message, std::time_t when) {
        if (!message) {
            cout << "can not add nullptr message" << endl;
            return false;
        }
        {
            lock_guard<mutex> lock(_messages_cv_m);
            if (_quit_flag) {
                message.reset();
                cout << "sending message to a Handler on a dead thread" << endl;
                return false;
            }
            message->mWhen = when;
            shared_ptr<Message> p = _messages;
            bool needWake = false;
            if (!p || message->mWhen < p->mWhen) {
                // New head, wake up the event queue if blocked.
                message->mNext = p;
                _messages = message;
                needWake = mBlocked;
            } else {
                shared_ptr<Message> prev;
                for (;;) {
                    prev = p;
                    p = p->mNext;
                    if (!p || when < p->mWhen) {
                        break;
                    }
                }
                message->mNext = p;
                prev->mNext = message;
            }

            if(needWake){
                wake();
            }
        }
        return true;
    }

    std::shared_ptr<Message> MessageQueue::next() {
        int nextPollTimeoutMillis = 0;
        for (;;) {
            pollOnce(nextPollTimeoutMillis);
            {
                lock_guard<mutex> lock(_messages_cv_m);
                time_t now = getCurrentTimeStamp();
                shared_ptr<Message> prevMsg;
                shared_ptr<Message> msg = _messages;
                if (msg) {
                    if(now < msg->mWhen){
                        nextPollTimeoutMillis = msg->mWhen - now;
#ifdef DEBUG
                        cout<<"now < msg->mWhen : nextPollTimeoutMillis "<<nextPollTimeoutMillis<<endl;
#endif
                    }else{
                        // Got a message
                        mBlocked = false;
                        _messages = msg->mNext;
                        msg->mNext = nullptr;
//                        cout << "msg[" << msg->mWhat << "] count " << msg.use_count() << endl;
                        return msg;
                    }
                } else {
//                    cout << "message queue is empty" << endl;
                    nextPollTimeoutMillis = -1;

                }
                if(_quit_flag){
                    return nullptr;
                }

                mBlocked = true;
                continue;
            }

        }


    }

    void MessageQueue::quit(bool safe) {
        {
            lock_guard<mutex> lock(_messages_cv_m);
            if (_quit_flag) {
                return;
            }
            _quit_flag = true;
            if (safe) {
                cout << "safe quit" << endl;
            } else {
                cout << "not safe quit" << endl;
            }
            wake();
        }
    }

    void MessageQueue::wake() {
        uint64_t inc = 1;
        ssize_t nWrite = write(mWakeEventFd,&inc,sizeof(uint64_t));
        if(nWrite != sizeof(uint64_t)){
            cout<<"Could not write wake signal to fd "<<mWakeEventFd<<": "<<strerror(errno)<<endl;
        }
    }

    void MessageQueue::pollOnce(int timeoutMillis) {
        struct epoll_event eventItems[8];
        int eventCount = epoll_wait(mEpollFd,eventItems,8,timeoutMillis);
        if(eventCount < 0){
//            cout<<"eventCount < 0"<<endl;
        }else if(eventCount == 0){
//            cout<<"timeout"<<endl;
        }else{
//            cout<<"eventCount is "<<eventCount<<endl;
        }
    }

    void MessageQueue::removeMessages(int what) {
        lock_guard<mutex> lock(_messages_cv_m);
        shared_ptr<Message> p = _messages;
        // Remove all messages at front.
        while(p&& p->mWhat == what){
            shared_ptr<Message> n = p->mNext;
            _messages = n;
            p->mNext = nullptr;
            p = n;
        }

        // Remove all messages after front.
        while(p){
            shared_ptr<Message> n = p->mNext;
            if(n&& n->mWhat == what){
                shared_ptr<Message> nn = n->mNext;
                n->mNext = nullptr;
                p->mNext = nn;
                continue;
            }
            p = n;
        }
    }
    //Looper
    Looper::Looper() {
#ifdef DEBUG
        cout<<"Create Looper"<<endl;
#endif
        mQueue = std::make_shared<MessageQueue>();
    }

    Looper::~Looper() {
#ifdef DEBUG
        cout<<"Destroy Looper"<<endl;
#endif
    }

    void Looper::loop() {
        if(!mQueue) return;
        for(;;){
            SptrMessage msg = mQueue->next();
            if(msg){
                if(msg->mTarget){
                    msg->mTarget->dispatchMessage(msg);
                    msg.reset();
                }else{
                    cout<<"msg target is null"<<endl;
                }

//                msg.reset();
            }else{
                return;
            }

        }
    }

    //Handler
    Handler::Handler(SptrLooper looper): mLooper(looper),mCallback(nullptr){
#ifdef DEBUG
        cout<<"create Handler"<<endl;
#endif
        mQueue = mLooper->mQueue;
    }
    Handler::Handler(SptrLooper looper, SptrCallback callback):
        mLooper(looper),
        mCallback(callback){
#ifdef DEBUG
        cout<<"create Handler"<<endl;
#endif
        mQueue = mLooper->mQueue;
    }

    Handler::~Handler() {
#ifdef DEBUG
        cout<<"destroy Handler"<<endl;
#endif
    }
    void Handler::dispatchMessage(SptrMessage msg) {
#ifdef DEBUG
        cout<<"Handler dispatchMessage "<<msg->mWhat<<endl;
#endif
        if(msg->mCallback){
            msg->mCallback->run();
        }else{
            if(mCallback){
                if(mCallback->handleMessage(msg)){
                    return;
                }
            }
            handleMessage(msg);
        }
    }

    bool Handler::sendEmptyMessage(int what) {
        return sendEmptyMessageDelayed(what,0);
    }

    SptrMessage Handler::getPostMessage(SptrRunnable r) {
        SptrMessage m = Message::obtain();
        m->mCallback = r;
        m->mTarget = shared_from_this();
        return m;
    }

    bool Handler::sendEmptyMessageDelayed(int what, long delayMillis) {
        SptrMessage msg = Message::obtain(shared_from_this(),what);
        return sendMessageDelayed(msg,delayMillis);
    }

    bool Handler::sendMessageAtTime(SptrMessage msg, std::time_t uptimeMillis) {
        return mQueue->enqueueMessage(msg,uptimeMillis);
    }

    bool Handler::sendMessageDelayed(SptrMessage msg, long delayMillis) {
        if(delayMillis < 0){
            delayMillis =  0;
        }
        return sendMessageAtTime(msg,getCurrentTimeStamp()+delayMillis);
    }

    bool Handler::sendMessage(SptrMessage msg) {
        return sendMessageDelayed(msg,0);
    }

    bool Handler::post(SptrRunnable runnable) {
        SptrMessage msg = getPostMessage(runnable);
        return sendMessage(msg);
    }

    bool Handler::postDelayed(SptrRunnable runnable, long delayMillis) {
        SptrMessage msg = getPostMessage(runnable);
        return sendMessageDelayed(msg,delayMillis);
    }

    //HandlerThread
    HandlerThread::HandlerThread(const std::string &name) :mName(name),mTid(-1){
#ifdef DEBUG
        cout<<"Create HandlerThread["<<mName<<"]"<<endl;
        #endif
        mLooper = std::make_shared<Looper>();
        lock_guard<mutex> lock(mMutex);
        mState = START;

    }

    HandlerThread::~HandlerThread() {
#ifdef DEBUG
        cout<<"Destroy HandlerThread["<<mName<<"]"<<endl;
#endif
    }

    void HandlerThread::start() {
#ifdef DEBUG
        printf("HandlerThread %s start\n",mName.c_str());
#endif

        mThread = make_shared<thread>(&HandlerThread::run,this);
        {
            lock_guard<mutex> lock(mMutex);
            mState = RUNNING;
        }
        mCv.notify_all();
    }

    bool HandlerThread::quit() {
#ifdef DEBUG
        printf("HandlerThread::quit\n");
#endif
        {
            lock_guard<mutex> lock(mMutex);
            mState = QUIT;
        }
        if(mLooper){
            mLooper->quite();
        }
        if(mThread){
            mThread->join();
            mThread.reset();
        }
    }

    void HandlerThread::run() {
        mTid = this_thread::get_id();
#ifdef DEBUG
        cout<<"HandlerThread["<<mName<<"]:run tid "<<mTid<<endl;
#endif
        mLooper->loop();
    }

    SptrLooper HandlerThread::getLooper() {
        unique_lock<mutex> lock;
        mCv.wait(lock,[&]{return mState == RUNNING;});
        return mLooper;
    }


}

