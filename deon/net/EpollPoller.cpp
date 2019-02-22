#include "../base/Logging.h"
#include "EpollPoller.h"
#include <sys/epoll.h>
#include <assert.h>

const int EPOLLWAIT_TIME = 10000;

EpollPoller::EpollPoller()
    : epollfd_(epoll_create1(EPOLL_CLOEXEC)),
      events_(InitEventListSize)
{
    if(epollfd_ < 0)
    {
        LOG << "EPollPoller::EPollPoller epoll_create1() error";
    }
}

void EpollPoller::poll(ChannelList &activeChannels)
{
    int numEvents = epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), EPOLLWAIT_TIME);
    if(numEvents > 0)
    {
        fillActiveChannels(numEvents, activeChannels);
    }
    else if(numEvents < 0)
    {
        LOG << "EPollPoller::poll() error";
    }
}

void EpollPoller::fillActiveChannels(int numEvents, ChannelList &activeChannels)
{
    assert(static_cast<size_t>(numEvents) <= events_.size());
    for(int i = 0; i < numEvents; i++)
    {
        //Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        int fd = events_[i].data.fd;
        //std::shared_ptr<Channel> channel = channels_[fd];
        std::map<int, std::shared_ptr<Channel>>::iterator it = channels_.find(fd);
        if(it != channels_.end())
        {
            std::shared_ptr<Channel> channel = it->second;
            channel->setRevents(events_[i].events);
            activeChannels.push_back(it->second);
        }
    }
}

void EpollPoller::addChannel(std::shared_ptr<Channel> channel, int timeout)
{
    int fd = channel->fd();
    channels_[fd] = channel;
    if(timeout > 0)
    {
        addTimer(channel, timeout);
        https_[fd] = channel->getTie();
    }
    update(EPOLL_CTL_ADD, channel, false);
}

void EpollPoller::updateChannel(std::shared_ptr<Channel> channel, int timeout)
{
    if(timeout > 0)
    {
        addTimer(channel, timeout);
    }
    update(EPOLL_CTL_MOD, channel, false);
}

void EpollPoller::deleteChannel(std::shared_ptr<Channel> channel)
{
    int fd = channel->fd();
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    channels_[fd].reset();
    https_[fd].reset();
    update(EPOLL_CTL_DEL, channel, true);
}

void EpollPoller::update(int operation, std::shared_ptr<Channel> channel, bool isDelete)
{
    int fd = channel->fd();
    struct epoll_event event;
    event.data.fd = fd;
    event.events = channel->events();
    if(epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if(!isDelete)
        {
            channels_.erase(fd);
        }
    }
}

const char* EpollPoller::operationToString(int op)
{
    switch (op)
    {
        case EPOLL_CTL_ADD:
            return "ADD";
        case EPOLL_CTL_DEL:
            return "DEL";
        case EPOLL_CTL_MOD:
            return "MOD";
        default:
            return "Unknown Operation";
    }
}

void EpollPoller::addTimer(std::shared_ptr<Channel> channel, int timeout)
{
    std::shared_ptr<Http> p = channel->getTie();
    if(p)
        timerManager_.addTimer(p, timeout);
    else
        LOG << "add timer failed ";
}
