//
// Created by Will on 2021/8/28.
//

#ifndef CPP_TOOLS_HANDLER_H
#define CPP_TOOLS_HANDLER_H
#include <memory>
#include <chrono>
#include <mutex>
#include <condition_variable>

namespace bs_tools{

    using time_stamp = std::chrono::time_point<std::chrono::system_clock,std::chrono::milliseconds>;
    std::time_t getCurrentTimeStamp();

    class Runnable{
    public:
        virtual void run() = 0;
        virtual ~Runnable() = default;
    };

    class MessageQueue;

    class Message{
    public:
        Message();

    public:
        std::time_t _when;
        int _what;
        std::shared_ptr<Runnable> _sptr_runnable;
    private:
        Message* _next;
        friend class MessageQueue;
    };

    class MessageQueue{
    public:
        bool enqueueMessage(std::shared_ptr<Message> message,long when);

        std::shared_ptr<Message> next();

    private:
        std::shared_ptr<Message>  _messages;
        bool _quit_flag = false;
        std::mutex _messages_cv_m;
        std::condition_variable _messages_cv;
    };


}

#endif //CPP_TOOLS_HANDLER_H
