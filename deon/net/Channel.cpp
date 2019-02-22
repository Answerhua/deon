#include "../base/Logging.h"
#include "Channel.h"
#include "EventLoop.h"
#include <sys/epoll.h>

Channel::Channel(EventLoop *loop)
    : loop_(loop),
      revents_(0)
{
}

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop),
      fd_(fd),
      revents_(0)
{
}

Channel::~Channel()
{
}

void Channel::handleEvents()
{
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        LOG << "fd = " << fd_ << " Channel::handle_event() POLLHUP";
    }
    if(revents_ & EPOLLERR)
    {
        if(errorCallback_) errorCallback_();
    }
    if(revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
    {
        if(readCallback_) readCallback_();
    }
    if(revents_ & EPOLLOUT)
    {
        if(writeCallback_) writeCallback_();
    }
    registerCallback_();
};
