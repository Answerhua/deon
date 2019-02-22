#include "AsyncLogging.h"
#include "LogFile.h"
#include <string>
#include <assert.h>


AsyncLogging::AsyncLogging(std::string logFileName_, int flushInterval)
    : flushInterval_(flushInterval),
      running_(false),
      basename_(logFileName_),
      thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
      mutex_(),
      cond_(mutex_),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_(),
      latch_(1)
{
    assert(logFileName_.size() > 1);
    currentBuffer_->bzero();    //清空当前缓冲区
    nextBuffer_->bzero();       //清空下一块缓冲区
    buffers_.reserve(16);       //初始化buffer队列的大小为16
}

//添加日志
void AsyncLogging::append(const char* logline, int len)
{
    MutexLockGuard lock(mutex_);    //加锁
    //如果当前日志还有空间，就添加到当前日志
    if(currentBuffer_->avail() > len)
        currentBuffer_->append(logline, len);
    //当前buffer已经满了
    else
    {
        buffers_.push_back(currentBuffer_);
        //重制当前buffer
        currentBuffer_.reset();
        if(nextBuffer_)
            currentBuffer_ = std::move(nextBuffer_);
        else
            currentBuffer_.reset(new Buffer);
        currentBuffer_->append(logline, len);   //继续添加
        //通知日志线程有数据可写（当前buffer满了的情况下）
        cond_.notify();
    }
}

void AsyncLogging::threadFunc()
{
    assert(running_ == true);
    latch_.countDown();
    
    LogFile output(basename_);  //日志对象文件
    
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    
    newBuffer1->bzero();
    newBuffer1->bzero();
    
    BufferVector buffersToWrite;    //需要写入文件的buffer
    buffersToWrite.reserve(16);
    
    while(running_)
    {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());

        {
            MutexLockGuard lock(mutex_);    //加锁
            
            // 如果buffer为空，那么表示没有数据需要写入文件，那么就等待指定的时间（注意这里没有用倒数计数器）
            if(buffers_.empty())
            {
                cond_.waitForSeconds(flushInterval_);
            }
            buffers_.push_back(currentBuffer_);
            currentBuffer_.reset();
            currentBuffer_ = std::move(newBuffer1);
            buffersToWrite.swap(buffers_);
            if(!nextBuffer_)
            {
                nextBuffer_ = std::move(newBuffer2);
            }
        }

        assert(!buffersToWrite.empty());

        // 如果buffer列表中的buffer个数大于25，将多余数据删除(避免出现信息堆积)
        if(buffersToWrite.size() > 25)
        {
            buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
        }

        // 将bufferToWrite的数据写入到日志中
        for(size_t i = 0; i < buffersToWrite.size(); ++i)
        {
            output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());
        }

        if(buffersToWrite.size() > 2)
        {
            //重置buffersToWrite大小，用于newBuffer1 2
            buffersToWrite.resize(2);
        }

        // 将buffer归还给newBuffer1 2
        if(!newBuffer1)
        {
            assert(!buffersToWrite.empty());
            newBuffer1 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if(!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear(); //清理
        
        output.flush(); //冲刷
    }
    output.flush();
}
