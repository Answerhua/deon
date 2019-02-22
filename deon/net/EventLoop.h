#ifndef NET_EVENTLOOP_H
#define NET_EVENTLOOP_H

#include "../base/CurrentThread.h"
#include "../base/MutexLock.h"
#include "Channel.h"
#include "EpollPoller.h"
#include <vector>

class EventLoop
{
    public:
        typedef std::function<void()> Functor;

        EventLoop();
        ~EventLoop();
        void loop();
        void quit();
        void runInLoop(Functor cb);
        void queueInLoop(Functor cb);
        void wakeup();  //唤醒事件通知描述符
        void addChannel(std::shared_ptr<Channel> channel, int timeout = 0);
        void updateChannel(std::shared_ptr<Channel> channel, int timeout = 0);
        void removeChannel(std::shared_ptr<Channel> channel);
        bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
    
    private:
        void handleRead();  //将事件通知符里的内容读走
        void handleRegister();
        void doPendingFunctors();   //执行转交给I/O的事务

        typedef std::vector<std::shared_ptr<Channel>> ChannelList;

        bool looping_;  //是否运行
        bool quit_; //是否退出循环
        bool eventHandling_;
        bool callingPendingFunctors_;
        const pid_t threadId_;  //运行loop的线程ID
        std::unique_ptr<EpollPoller> poller_;   //I/O复用
        int wakeupFd_;  //唤醒嵌套字
        std::shared_ptr<Channel> wakeupChannel_;    //封装事件描述符

        ChannelList activeChannels_;    //活跃的事件集

        mutable MutexLock mutex_;   //互斥锁
        std::vector<Functor> pendingFunctors_;  //需要在主I/O线程上执行的任务
};

#endif
