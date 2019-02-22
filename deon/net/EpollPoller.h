#ifndef NET_EPOLLPOLLER_H
#define NET_EPOLLPOLLER_H

#include "Channel.h"
#include "Timer.h"
#include <memory>
#include <vector>
#include <map>

const int InitEventListSize = 4096;

class EpollPoller
{
    public:
        typedef std::vector<std::shared_ptr<Channel> > ChannelList;
        
        EpollPoller();
        ~EpollPoller() = default;
        
        void poll(ChannelList &activeChannels);
        void addChannel(std::shared_ptr<Channel> channel, int timeout = 0);
        void updateChannel(std::shared_ptr<Channel> channel, int timeout = 0);
        void deleteChannel(std::shared_ptr<Channel> channel);
            
        int getEpollfd(){ return epollfd_;}
        void handleExpired(){ timerManager_.handleExpiredEvent(); }

    private:
        void update(int operation, std::shared_ptr<Channel> channel, bool isDelete);
        const char* operationToString(int op);
        void fillActiveChannels(int numEvents, ChannelList  &activeChannels) ;
        void addTimer(std::shared_ptr<Channel> channel, int timeout);
        int epollfd_;
        std::vector<epoll_event> events_;
        std::map<int, std::shared_ptr<Channel>> channels_;
        std::map<int, std::shared_ptr<Http>> https_;
        TimerManager timerManager_;
};

#endif
