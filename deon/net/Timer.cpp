#include "Timer.h"
#include <sys/time.h>
#include <queue>

TimerNode::TimerNode(std::shared_ptr<Http> http, size_t timeout)
    :   deleted_(false),
        http_(http)
{
    this->nodeUpdate(timeout);
}

TimerNode::~TimerNode()
{
    if(http_)
        http_->handleClose();
}

void TimerNode::nodeUpdate(size_t timeout)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    expiredTime_ = (((tv.tv_sec % 10000) * 1000) + (tv.tv_usec / 1000)) + timeout;
}

bool TimerNode::isValid()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    size_t tmp = (((tv.tv_sec % 10000) * 1000) + (tv.tv_usec / 1000));
    if (tmp < expiredTime_)
        return true;
    else
    {
        this->setDeleted();
        return false;
    }
}

void TimerNode::resetHttp()
{
    http_.reset();
}

void TimerManager::addTimer(std::shared_ptr<Http> httpData_, size_t timeout)
{
    TimerNode_ new_node(new TimerNode(httpData_, timeout));
    timerNodeQueue.push(new_node);
    httpData_->addTimer(new_node);  
}

void TimerManager::handleExpiredEvent()
{
    while(!timerNodeQueue.empty())
    {
        TimerNode_ top_node = timerNodeQueue.top();
        if (top_node->isValid() == false)
            timerNodeQueue.pop();
        else
            break;              
    }
}
