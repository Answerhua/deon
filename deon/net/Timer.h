#ifndef NET_TIMER_H
#define NET_TIMER_H

#include "Http.h"
#include <queue>
#include <memory>
#include <vector>

class TimerNode
{   
    public:
        TimerNode(std::shared_ptr<Http> request, size_t timeout);
        ~TimerNode();
        void nodeUpdate(size_t timeout);
        bool isValid();
        void resetHttp();
        void setDeleted() { deleted_ = true; }
        bool isDeleted() const { return deleted_; }
        size_t getExpTime() const { return expiredTime_; }
    private:
        bool deleted_;
        size_t expiredTime_;
        std::shared_ptr<Http> http_;    
};

struct TimerCmp
{
    bool operator()(std::shared_ptr<TimerNode> &a, std::shared_ptr<TimerNode> &b) const
    {
        return a->getExpTime() > b->getExpTime();
    }
};

class TimerManager
{
    public:
        TimerManager() = default;
        ~TimerManager() = default;
        void addTimer(std::shared_ptr<Http> http_, size_t timeout);
        void handleExpiredEvent();
    private:
        typedef std::shared_ptr<TimerNode> TimerNode_;
        std::priority_queue<TimerNode_, std::vector<TimerNode_>, TimerCmp> timerNodeQueue;
};

#endif
