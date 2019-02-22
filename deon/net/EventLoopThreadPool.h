#ifndef NET_EVENTLOOPTHREADPOOL_H
#define NET_EVENTLOOPTHREADPOOL_H

#include "EventLoopThread.h"
#include <memory>
#include <vector>

class EventLoopThreadPool 
{
    public:
        EventLoopThreadPool(EventLoop* baseLoop, int numThreads);
        ~EventLoopThreadPool();
        void start();
        EventLoop *getNextLoop();
    
    private:
        EventLoop* baseLoop_;
        bool started_;
        int numThreads_;    //线程数
        int next_;  //新连接到来，所选择的EventLoop的下标
        std::vector<std::unique_ptr<EventLoopThread>> threads_; //I/O线程列表
        std::vector<EventLoop*> loops_; //EventLoop列表
};

#endif
