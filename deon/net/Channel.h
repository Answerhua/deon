#ifndef NET_CHANNEL_H
#define NET_CHANNEL_H

#include <functional>
#include <memory>
#include <sys/epoll.h>

class EventLoop;
class Http;

class Channel
{
    public:
        typedef std::function<void()> EventCallback;

        Channel(EventLoop* loop);   
        Channel(EventLoop* loop, int fd);
        ~Channel();

        void handleEvents();

        void setReadCallback(EventCallback cb)
        { readCallback_ = std::move(cb); }
        void setWriteCallback(EventCallback cb)
        { writeCallback_ = std::move(cb); }
        void setCloseCallback(EventCallback cb)
        { closeCallback_ = std::move(cb); }
        void setRegisterCallback(EventCallback cb)
        { registerCallback_ = std::move(cb); }
        void setErrorCallback(EventCallback cb)
        { errorCallback_ = std::move(cb); }

        void tie(std::shared_ptr<Http> tie) {tie_ = tie;}
        std::shared_ptr<Http> getTie() { return tie_.lock();}   
        
        int fd() const { return fd_; }
        __uint32_t events() const { return events_; }
        void setRevents(__uint32_t revents) { revents_ = revents;}
        void setEvents(__uint32_t events) { events_ = events;}
        void addEvents(__uint32_t events) { events_ |= events;}
        
        void remove();
        
    private:
        EventLoop* loop_;   //channel所属的EventLoop
        int  fd_;   //channel负责的文件描述符
        int  events_;   //注册的事件
        int  revents_;  //返回的就绪事件
        std::weak_ptr<Http> tie_;   //绑定http
        bool addedToLoop_;  //是否被添加进loop
        EventCallback readCallback_;
        EventCallback writeCallback_;
        EventCallback closeCallback_;
        EventCallback registerCallback_;  //ET模式下，完成操作后，需要重新注册
        EventCallback errorCallback_;
};

#endif
