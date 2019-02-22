#ifndef LOG_COUNTDOWNLATCH_H
#define LOG_COUNTDOWNLATCH_H

#include "Condition.h"
#include "MutexLock.h"

class CountDownLatch : noncopyable
{
    public:
        explicit CountDownLatch(int count);
        void wait();
        void countDown();
    
    private:
        mutable MutexLock mutex_;
        Condition condition_;
        int count_;
};

#endif
