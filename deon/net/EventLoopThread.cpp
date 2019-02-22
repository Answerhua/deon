#include "EventLoopThread.h"
#include <functional>

EventLoopThread::EventLoopThread()
    : loop_(NULL),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), "EventLoopThread"),
      mutex_(),
      cond_(mutex_)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != NULL)
    {   
        loop_->quit();  //退出I/O线程
        thread_.join(); //等待线程退出
    }
}

EventLoop* EventLoopThread::startLoop()
{
    assert(!thread_.started());
    thread_.start();    //线程启动，调用threadFunc
    {
        MutexLockGuard lock(mutex_);
        while (loop_ == NULL)
            cond_.wait();   //等待对象的创建
    }
    return loop_;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;
    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.notify(); //创建好对象，发送通知
    }
    loop.loop();
    MutexLockGuard lock(mutex_);
    loop_ = NULL;
}
