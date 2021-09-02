//
// Created by Will on 2021/8/28.
//

#ifndef CPP_TOOLS_HANDLER_H
#define CPP_TOOLS_HANDLER_H
#include <memory>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <thread>
//#define DEBUG
namespace bs_tools{

    using time_stamp = std::chrono::time_point<std::chrono::system_clock,std::chrono::milliseconds>;
    std::time_t getCurrentTimeStamp();

    class Message;
    using SptrMessage = std::shared_ptr<Message>;
    class Handler;
    using SptrHandler = std::shared_ptr<Handler>;
    class Looper;
    using SptrLooper = std::shared_ptr<Looper>;
    class Callback;
    using SptrCallback = std::shared_ptr<Callback>;
    class MessageQueue;
    using SptrMessageQueue = std::shared_ptr<MessageQueue>;
    class Runnable;
    using SptrRunnable = std::shared_ptr<Runnable>;

    class Runnable{
    public:
        virtual void run() = 0;
        virtual ~Runnable() = default;
    };

    class MessageQueue;

    class Message{
    public:
        Message();
        ~Message();

    public:
        std::time_t mWhen;
        int mWhat;
        std::shared_ptr<Runnable> mCallback;
        static SptrMessage obtain(SptrHandler handler,int what);
        static SptrMessage obtain();
        SptrHandler mTarget;
    private:
        std::shared_ptr<Message> mNext;

        friend class Handler;
        friend class MessageQueue;
    };

    class MessageQueue{
    public:
        MessageQueue();
        ~MessageQueue();
        bool enqueueMessage(std::shared_ptr<Message> message,std::time_t when);
        std::shared_ptr<Message> next();
        void removeMessages(int what);
        void quit(bool safe);

    private:
        void pollOnce(int timeoutMillis);
        void wake();

    private:
        std::shared_ptr<Message>  _messages;
        bool _quit_flag = false;
        std::mutex _messages_cv_m;
        std::condition_variable _messages_cv;
        bool mBlocked = false;

        int mEpollFd = -1;
        int mWakeEventFd = -1;
    };

    class Looper{
    public:
        Looper();
        ~Looper();
        void loop();
        void quite(){
            mQueue->quit(false);
        }
        void quiteSafely(){
            mQueue->quit(true);
        }
    private:
        std::shared_ptr<MessageQueue> mQueue;
        friend class Handler;


    };

    class Callback{
    public:
        Callback() = default;
        virtual bool handleMessage(SptrMessage msg) = 0;
        virtual ~Callback() = default;
    };

    class Handler:public std::enable_shared_from_this<Handler>{
    public:
        explicit Handler(SptrLooper looper);
        Handler(SptrLooper looper,SptrCallback callback);
        virtual ~Handler();
        void dispatchMessage(SptrMessage msg);
        bool sendEmptyMessage(int what);
        bool sendEmptyMessageDelayed(int what,long delayMillis);
        bool sendMessage(SptrMessage msg);
        bool sendMessageDelayed(SptrMessage msg,long delayMillis);
        bool post(SptrRunnable runnable);
        bool postDelayed(SptrRunnable runnable,long delayMillis);



        bool sendMessageAtTime(SptrMessage msg,std::time_t uptimeMillis);

        virtual void handleMessage(SptrMessage msg){}

    private:
        SptrMessage getPostMessage(SptrRunnable r);
    private:
        SptrLooper mLooper;
        SptrCallback mCallback;
        SptrMessageQueue mQueue;

    };

    class HandlerThread{
    public:
        HandlerThread(const std::string & name);
        ~HandlerThread();
        void start();
        bool quit();
        SptrLooper getLooper();

        enum {
            UNKNOWN = 0,
            START,
            RUNNING,
            QUIT
        };

    private:
        void run();
    private:
        std::shared_ptr<Looper> mLooper;
        std::shared_ptr<std::thread> mThread;
        std::string mName;
        std::thread::id mTid;
        std::mutex mMutex;
        std::condition_variable mCv;
        int mState = UNKNOWN;

    };


}

#endif //CPP_TOOLS_HANDLER_H
