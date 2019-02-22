#include "../base/Logging.h"
#include "../base/CurrentThread.h"
#include "../base/MutexLock.h"
#include "Util.h"
#include "EventLoop.h"
#include "EpollPoller.h"
#include <sys/eventfd.h>
#include <assert.h>
#include <unistd.h>

__thread EventLoop* t_loopInThisThread = 0;

int createEventfd()
{
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG << "Failed in create eventfd";
        abort();
    }
    return evtfd;
}

EventLoop::EventLoop()  //初始化事件循环
    :   looping_(false),
        quit_(false),
        eventHandling_(false),
        callingPendingFunctors_(false),
        threadId_(CurrentThread::tid()),
        poller_(new EpollPoller()),
        wakeupFd_(createEventfd()),
        wakeupChannel_(new Channel(this, wakeupFd_))
{
    if(t_loopInThisThread)
    {
        LOG << "Another EventLoop " << t_loopInThisThread
            << " exists in thie thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setEvents(EPOLLIN | EPOLLET);
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->setRegisterCallback(std::bind(&EventLoop::handleRegister, this));
    poller_->addChannel(wakeupChannel_);
}

EventLoop::~EventLoop()
{

}
    
void EventLoop::loop()
{
    assert(!looping_);
    assert(isInLoopThread());
    looping_ = true;
    quit_ = false;
    while(!quit_)
    {
        activeChannels_.clear();    //删除列表中所有元素
        poller_->poll(activeChannels_);
        eventHandling_ = true;
        //处理就绪事件
        for(auto &channel : activeChannels_)
        {
            channel->handleEvents();
        }
        eventHandling_ = false;
        doPendingFunctors();    //处理其他任务
    }
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    if(!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }

    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

void EventLoop::addChannel(std::shared_ptr<Channel> channel, int timeout)
{
    poller_->addChannel(channel, timeout);
}
void EventLoop::updateChannel(std::shared_ptr<Channel> channel, int timeout)
{
    poller_->updateChannel(channel, timeout);
}
void EventLoop::removeChannel(std::shared_ptr<Channel> channel)
{
    poller_->deleteChannel(channel);
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = writen(wakeupFd_, (char*)(&one), sizeof one);   //通过向eventfd中写字节，唤醒
    if (n != sizeof one)
    {
        LOG << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = readn(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRegister()
{
    poller_->addChannel(wakeupChannel_);
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);    //减少临界区长度
    }
    
    for (const Functor& functor : functors)
    {
        functor();
    }
    callingPendingFunctors_ = false;
}
