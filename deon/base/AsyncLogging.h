#ifndef LOG_ASYNCLOGGING_H
#define LOG_ASYNCLOGGING_H

#include "CountDownLatch.h"
#include "MutexLock.h"
#include "Thread.h"
#include "LogStream.h"
#include <string>
#include <vector>

class AsyncLogging : noncopyable
{
    public:
        AsyncLogging(const std::string basename, int flushInterval = 2);
        ~AsyncLogging()
        {
            if(running_)
                stop();
        }
        void append(const char* logline, int len);
        
        //开始启动异步日志      
        void start()
        {
            running_ = true;
            thread_.start();
            latch_.wait();
        }

        //停止异步日志
        void stop()
        {
            running_ = false;
            cond_.notify();
            thread_.join();
        }

    private:
        void threadFunc();  // 线程调用的函数，主要用于周期性的flush数据到日志文件中
        typedef FixedBuffer<kLargeBuffer> Buffer;
        typedef std::vector<std::shared_ptr<Buffer>> BufferVector;  //Buffer队列
        typedef std::shared_ptr<Buffer> BufferPtr;  //Buffer指针
        const int flushInterval_;   //日志线程等待的时间长度
        bool running_;  //是否正在运行
        std::string basename_;  //日志名字
        Thread thread_; //记录该异步日志记录器的线程
        MutexLock mutex_;
        Condition cond_;
        BufferPtr currentBuffer_;   //当前的缓冲区
        BufferPtr nextBuffer_;  //下一个缓冲区
        BufferVector buffers_;
        CountDownLatch latch_;  
};

#endif
