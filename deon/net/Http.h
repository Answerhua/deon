#ifndef NET_HTTP_H
#define NET_HTTP_H

#include "HttpRequest.h"
#include <string>
#include <iostream>

class Timer;

enum HttpRequestParseState
{
    STATE_PARSE_REQUESTLINE,
    STATE_PARSE_HEADERS,
    STATE_CHECK_CONTENT_LENGTH,
    STATE_ANALYSIS,
    STATE_FINISH
};
        
enum LineState
{
    PARSE_LINE_SUCCESS,
    PARSE_LINE_AGAIN,
    PARSE_LINE_ERROR
};

enum HeaderState
{
    PARSE_HEADER_SUCCESS,
    PARSE_HEADER_AGAIN,
    PARSE_HEADER_ERROR
};

enum HeaderParseState
{
    H_START,
    H_KEY,
    H_SPACES_AFTER_COLON,
    H_VALUE,
    H_CR,
    H_LF,
    H_END_CR
};

class Http: public std::enable_shared_from_this<Http>
{
    public:
        Http(EventLoop *loop, int connfd, std::string root);
        ~Http() { close(fd_); }
        void registerHttp();
        void handleClose();
        void addTimer(std::shared_ptr<TimerNode> timer) { timer_ = timer; }
        void deleteTimer();
        std::shared_ptr<Channel> getChannel() { return channel_; }

    private:
        void handleRead();
        void handleWrite();
        void handleError(int fd, int err_num, std::string short_msg);
        void handleRegister();
        void initParse();
        LineState parseRequestLine(std::string& http_request);
        HeaderState parseRequestHeader(std::string& http_request);
        int analysisRequest();
        EventLoop *loop_;
        std::shared_ptr<Channel> channel_;
        int fd_;
        std::string root_;
        bool err_;
        bool isConnecting_;
        bool keepAlive_;
        std::string inputBuffer_;
        std::string outputBuffer_;
        HttpRequestParseState state_;
        HeaderParseState hState_;
        HttpRequest request_;
        std::weak_ptr<TimerNode> timer_;
};

#endif
