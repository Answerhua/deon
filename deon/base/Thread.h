#ifndef LOG_THREAD_H
#define LOG_THREAD_H

#include "noncopyable.h"
#include "CountDownLatch.h"
#include <functional>
#include <memory>
#include <pthread.h>
#include <string>

class Thread : noncopyable
{
    public:
        typedef std::function<void ()> ThreadFunc;
        explicit Thread(const ThreadFunc&, const std::string& name = std::string());
        ~Thread();
        void start();
        int join(); // 返回pthread_join()
        bool started() const { return started_; }
        pid_t tid() const { return tid_; } ////由于pthread_t的id号可能是一样的，所以需要用gettid()
        const std::string& name() const { return name_; }

    private:
        void setDefaultName();
        bool started_;
        bool joined_;
        pthread_t pthreadId_;
        pid_t tid_;
        ThreadFunc func_; //线程回调函数
        std::string name_;
        CountDownLatch latch_;
};

#endif
