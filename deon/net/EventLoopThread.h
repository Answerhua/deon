#ifndef NET_EVENTLOOPTHREAD_H
#define NET_EVENTLOOPTHREAD_H

#include "../base/Thread.h"
#include "../base/MutexLock.h"
#include "EventLoop.h"

class EventLoopThread
{
    public:
        EventLoopThread();
        ~EventLoopThread();
        EventLoop* startLoop(); //启动线程

    private:
        void threadFunc();  //线程函数

        EventLoop *loop_;   //loop_指向一个EventLoop对象
        bool exiting_;
        Thread thread_;
        MutexLock mutex_;
        Condition cond_;
};

#endif
