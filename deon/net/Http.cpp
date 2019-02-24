#include "../base/Logging.h"
#include "EventLoop.h"
#include "Channel.h"
#include "Util.h"
#include "Http.h"
#include "HttpRequest.h"
#include <unordered_map>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <iostream>

const int DEFAULT_EXPIRED_TIME = 2000;
const int DEFAULT_KEEP_ALIVE_TIME = 5 * 60 * 1000; 

std::unordered_map<std::string, std::string> mime_type = 
{
    {".html" , "text/html"},
    {".avi" , "video/x-msvideo"},
    {".bmp" , "image/bmp"},
    {".c" , "text/plain"},
    {".doc" , "application/msword"},
    {".gif" , "image/gif"},
    {".gz" , "application/x-gzip"},
    {".htm" , "text/html"},
    {".ico" , "image/x-icon"},
    {".jpg" , "image/jpeg"},
    {".png" , "image/png"},
    {".txt" , "text/plain"},
    {".mp3" , "audio/mp3"},
    {"default" , "text/html"}
};

Http::Http(EventLoop *loop, int connfd, std::string root)
    : loop_(loop),
      channel_(new Channel(loop, connfd)),
      fd_(connfd),
      root_(root),
      err_(false),
      isConnecting_(true),
      keepAlive_(false),
      state_(STATE_PARSE_REQUESTLINE),
      hState_(H_START)
{
    channel_->setReadCallback(std::bind(&Http::handleRead, this));
    channel_->setWriteCallback(std::bind(&Http::handleWrite, this));
    channel_->setCloseCallback(std::bind(&Http::handleClose, this));
    channel_->setRegisterCallback(std::bind(&Http::handleRegister, this));
}

void Http::registerHttp()
{
    channel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
    loop_->addChannel(channel_, DEFAULT_EXPIRED_TIME);
}

void Http::handleRead()
{
    bool finish = false;
    do
    {
        bool zero = false;
        int read_num = readn(fd_, inputBuffer_, zero);
        LOG << "Request from fd:" << fd_ << " \n" << inputBuffer_;
        if (read_num < 0)
        {
            err_ = true;
            handleError(fd_, 400, "Bad Request");
            break;
        }
        if(zero)
        {
            isConnecting_ = false;
            break; 
        }
        else if(read_num == 0)
        {
            //读到EAGIN，且无数据可读
            finish = true;
            break;
        }
        if (state_ == STATE_PARSE_REQUESTLINE)
        {
            LineState rc = this->parseRequestLine(inputBuffer_);
            if(rc == PARSE_LINE_AGAIN)
            {
                finish = true;
                break;
            }
            else if(rc == PARSE_LINE_ERROR)
            {
                LOG << "fd:" << fd_ << " PARSE_LINE_ERROR";
                err_ = true;
                handleError(fd_, 400, "Bad Request");
                break;
            }
            else
            {
                state_ = STATE_PARSE_HEADERS;
            }
        }

        if (state_ == STATE_PARSE_HEADERS)
        {
            HeaderState rc = this->parseRequestHeader(inputBuffer_);
            if(rc == PARSE_HEADER_AGAIN)
            {
                LOG << "fd:" << fd_ << " Parse header again";
                finish = true;
                break;
            }   
            else if(rc == PARSE_HEADER_ERROR)
            {
                LOG << "fd:" << fd_ << " PARSE_HEADER_ERROR";
                err_ = true;
                handleError(fd_, 400, "Bad Request");
                break;  
            }
            Method method = request_.getMethod();
            if(method == M_POST)
            {
                state_ = STATE_CHECK_CONTENT_LENGTH;
            }
            else
            {
                state_ = STATE_ANALYSIS;
            }
        }

        if(state_ == STATE_CHECK_CONTENT_LENGTH)
        {
            int content_length = -1;
            std::map<std::string, std::string> headers_ = request_.getHeaders();
            if (headers_.find("Content-length") != headers_.end())
            {
                content_length = stoi(headers_["Content-length"]);
            }
            else
            {
                err_ = true;
                handleError(fd_, 400, "Bad Request: Lack of (Content-length)");
                break;
            }
            if (static_cast<int>(inputBuffer_.size()) < content_length)
            {
                finish = true;
                break;
            }
            state_ = STATE_ANALYSIS;
        }

        if(state_ == STATE_ANALYSIS)
        {
            int rc = this->analysisRequest();
            if(rc == 0)
            {
                state_ = STATE_FINISH;
                break;
            }
            else
            {
                err_ = true;
                break;
            }
        }
    }while(true);

    if(!err_)
    {
        if(outputBuffer_.size() > 0)
        {
            handleWrite();
        }
        if(finish)
        {
            channel_->addEvents(EPOLLIN);
        }
        else if(!err_ && state_ == STATE_FINISH)
        {
            this->initParse();
            if(isConnecting_ && (!inputBuffer_.empty()))
            {
                handleRead();
            }
        }
    }
}

void Http::handleWrite()
{
    if(!err_ && isConnecting_)
    { 
        LOG << "Response to fd:" << fd_ << " \n" << outputBuffer_;
        if(writen(fd_, outputBuffer_) < 0)
        {
            err_ = true;
        }
        if(!outputBuffer_.empty())
        {
            channel_->addEvents(EPOLLOUT);
        }
    }   
}

void Http::handleError(int fd, int err_num, std::string short_msg)
{
    char send_buff[4096];
    std::string body_buff, header_buff;
    body_buff += "<html><title>Server error</title>";
    body_buff += "<body bgcolor=\"ffffff\">";
    body_buff += std::to_string(err_num) + short_msg;
    body_buff += "<hr><em> Deon</em>\n</body></html>";

    header_buff += "HTTP/1.1 " + std::to_string(err_num) + short_msg + "\r\n";
    header_buff += "Server: Answerhua's Web Server\r\n";
    header_buff += "Content-Type: text/html\r\n";
    header_buff += "Connection: Close\r\n";
    header_buff += "Content-Length: " + std::to_string(body_buff.size()) + "\r\n";
    header_buff += "\r\n";
    
    sprintf(send_buff, "%s", header_buff.c_str());
    writen(fd, send_buff, strlen(send_buff));
    sprintf(send_buff, "%s", body_buff.c_str());
    writen(fd, send_buff, strlen(send_buff));
}

void Http::handleClose()
{
    loop_->removeChannel(channel_);
}

void Http::handleRegister()
{
    deleteTimer();
    __uint32_t events = channel_->events();
    if(!err_ && isConnecting_)
    {
        channel_->setEvents(0);
        int timeout = DEFAULT_EXPIRED_TIME;
        if(keepAlive_)
        {
            timeout = DEFAULT_KEEP_ALIVE_TIME;
        }
        if(events & EPOLLOUT)
        {
            channel_->addEvents(EPOLLET | EPOLLOUT);
        }
        else
        {
            channel_->addEvents(EPOLLET | EPOLLIN);
        }
        loop_->updateChannel(channel_, timeout);
    }
    else
    {
        handleClose();
    }
}

void Http::initParse()
{
    state_ = STATE_PARSE_REQUESTLINE;
    hState_ = H_START;
    request_.requestInit();
}

void Http::deleteTimer()
{
    if(timer_.lock())
    {
        std::shared_ptr<TimerNode> timer(timer_);
        timer->resetHttp();
        timer->setDeleted();
        timer.reset();
    }
}

LineState Http::parseRequestLine(std::string& http_request)
{
    int pos = http_request.find("\r\n", 0);
    if(pos < 0)
    {
        return PARSE_LINE_AGAIN;
    }
    std::string request_line = http_request.substr(0, pos);
    if (static_cast<int> (http_request.size()) > pos + 2)
    {
        http_request = http_request.substr(pos + 2);
    }
    else
    {
        http_request.clear();
    }
    
    int posGet = request_line.find("GET");
    int posPost = request_line.find("POST");
    int posHead = request_line.find("HEAD");

    if(posGet >= 0)
    {
        request_.setMethod(M_GET);
        pos = posGet;
    }
    else if(posPost >= 0)
    {
        request_.setMethod(M_POST);
        pos = posPost;
    }
    else if(posHead >= 0)
    {
        request_.setMethod(M_HEAD);
        pos = posHead;
    }
    else
    {
        LOG << "fd:" << fd_ << " Parse LINE error(Invalid METHOD)";
        return PARSE_LINE_ERROR;
    }
    pos = request_line.find('/', pos );
    //设置默认http版本号和uri
    if(pos < 0)
    {
        request_.setVersion(HTTP_11);
        request_.setUri("index.html");
        return PARSE_LINE_SUCCESS;
    }
    //进行http解析
    else
    {
        int endUriPos = request_line.find(' ', pos);
        if (endUriPos < 0)
        {
            LOG << "fd:" << fd_ << " Parse LINE error(Invalid URI)";
            return PARSE_LINE_ERROR;
        }
        else
        {
            std::string tmpFileName;
            tmpFileName = request_line.substr(pos + 1, endUriPos - pos - 1);
            endUriPos = tmpFileName.find('?');
            if (endUriPos >= 0)
            {
                tmpFileName = tmpFileName.substr(0, endUriPos);
            }
            if(tmpFileName.empty())
            {
                tmpFileName = "index.html";
            }
            request_.setUri(tmpFileName);
        }
        pos = request_line.find("/", pos + 1);
        if (pos < 0)
        {
            LOG << "fd:" << fd_ << " Parse LINE error(Invalid VERSION)";
            return PARSE_LINE_ERROR;
        }
        std::string version = request_line.substr(pos + 1, 3);
        if(version == "1.0")
        {
            request_.setVersion(HTTP_10);
        }
        else if (version == "1.1")
        {
            request_.setVersion(HTTP_11);
        }
        else
        {
            return PARSE_LINE_ERROR;
        }

    }
    return PARSE_LINE_SUCCESS;
}

HeaderState Http::parseRequestHeader(std::string& http_request)
{
    bool finish = false;
    int key_start = 0, key_end = 0, value_start = 0, value_end = 0, read_line_pos = 0;
    size_t i = 0;
    for(; i < http_request.size() && !finish; ++i)
    {
        switch(hState_)
        {
            case H_START:
            {
                if(http_request[i] == '\n' || http_request[i] == '\r')
                {
                    break;
                }
                hState_ = H_KEY;
                key_start = i;
                read_line_pos = i;
                break;      
            }
            case H_KEY:
            {
                if(http_request[i] == ':')
                {
                    key_end = i;
                    if(key_end - key_start <=0 )
                        return PARSE_HEADER_ERROR;
                    hState_ = H_SPACES_AFTER_COLON;
                }
                break;
            }
            case H_SPACES_AFTER_COLON:
            {
                if(http_request[i] == ' ')
                {
                    hState_ = H_VALUE;
                    value_start = i + 1;
                }
                else
                    return PARSE_HEADER_ERROR;
                break;
            }
            case H_VALUE:
            {
                if(http_request[i] == '\r')
                {
                    hState_ = H_CR;
                    value_end = i;
                    if (value_end - value_start <= 0)
                        return PARSE_HEADER_ERROR;
                }
                break;
            }
            case H_CR:
            {
                if(http_request[i] == '\n')
                {
                    hState_ = H_LF;
                    std::string key(http_request.begin() + key_start, http_request.begin() + key_end);
                    std::string value(http_request.begin() + value_start, http_request.begin() + value_end);
                    request_.setHeader(key, value);
                    read_line_pos = i;
                }
                else
                    return PARSE_HEADER_ERROR;
                break;
            }
            case H_LF:
            {
                if (http_request[i] == '\r')
                {
                    hState_ = H_END_CR;
                }
                else
                {
                    key_start = i;
                    hState_ = H_KEY;
                }
                break;
            }
            case H_END_CR:
            {
                if (http_request[i] == '\n')
                {
                    finish = true;
                    read_line_pos = i;
                    break;
                }
            }
        }
    }
    if (finish)
    {
        http_request = http_request.substr(i);
        return PARSE_HEADER_SUCCESS;
    }
    http_request = http_request.substr(read_line_pos);
    return PARSE_HEADER_AGAIN;  
}

int Http::analysisRequest()
{
    Method method = request_.getMethod();
    std::string fileName = root_ + request_.getUri();
    if (method == M_GET || method == M_HEAD)
    {
        std::string header;
        std::map<std::string, std::string> headers = request_.getHeaders();
        
        header += "HTTP/1.1 200 OK\r\n";
        if(headers.find("Connection") != headers.end() && (headers["Connection"] == "Keep-Alive" || headers["Connection"] == "keep-alive"))
        {
            keepAlive_ = true;
            header += std::string("Connection: Keep-Alive\r\n") + "Keep-Alive: timeout=" + std::to_string(DEFAULT_KEEP_ALIVE_TIME) + "\r\n";
        }
        
        int dotPos = fileName.find('.');
        std::string file_type;
        if(dotPos < 0)
            file_type = mime_type["default"];
        else
            file_type = mime_type[fileName.substr(dotPos)];
        if (fileName == "hello")
        {
            outputBuffer_ = "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\n\r\nHello World";
            return 0;
        }   

        struct stat sbuf;
        if (stat(fileName.c_str(), &sbuf) < 0)
        {
            handleError(fd_, 404, "Not Found!");
            return -1;
        }
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
        {
            handleError(fd_, 403, "Forbidden");
            return -1;
        }

        header += "Server: Answerhua's Web Server\r\n";
        header += "Content-Type: " + file_type + "\r\n";
        header += "Content-Length: " + std::to_string(sbuf.st_size) + "\r\n";
        
        header += "\r\n";
        outputBuffer_ += header;    //添加头部
        
        if(method == M_HEAD)
        {
            return 0;
        }

        int src_fd = open(fileName.c_str(), O_RDONLY, 0);
        if (src_fd < 0)
        {
            handleError(fd_, 404, "Not Found");
            return -1;
        }

        void *rc = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
        close(src_fd);
        
        if(rc == (void *)-1)
        {
            munmap(rc, sbuf.st_size);
        }
        
        char *src_addr = static_cast<char*>(rc);
        outputBuffer_ += std::string(src_addr, src_addr + sbuf.st_size);
        munmap(rc, sbuf.st_size);
        
        return 0;
    }
    return -1;
}
