#ifndef LOG_MUTEXLOCK_H
#define LOG_MUTEXLOCK_H

#include "noncopyable.h"
#include "CurrentThread.h"
#include <pthread.h>
#include <assert.h>

#define MCHECK(ret) ({ __typeof__ (ret) errnum = (ret);         \
                       assert(errnum == 0); (void) errnum;})

class MutexLock : noncopyable
{
    public:
        MutexLock()
            : holder_(0)
        {
            MCHECK(pthread_mutex_init(&mutex_, NULL));
        }
        
        ~MutexLock()
        {
            assert(holder_ == 0);   //只有在没有线程持有的情况下，才可以析构
            pthread_mutex_destroy(&mutex_);
        }
        
        bool isLockedByThisThread() const   //判断是否被本线程上锁
        {
            return holder_ == CurrentThread::tid();
        }

        void assertLocked() const
        {
            assert(isLockedByThisThread());
        }
        
        void lock()
        {
            MCHECK(pthread_mutex_lock(&mutex_));
            assignHolder(); //将holer_赋值pid
        }
        
        void unlock()
        {
            unassignHolder();  //取消赋值
            pthread_mutex_unlock(&mutex_);
        }
        
        pthread_mutex_t *get()
        {
            return &mutex_;
        }

    private:
        friend class Condition;
        void unassignHolder()
        {
            holder_ = 0;
        }

        void assignHolder()
        {
            holder_ = CurrentThread::tid();
        }
        pthread_mutex_t mutex_;
        pid_t holder_;
};

class MutexLockGuard : noncopyable //利用C++的RAII机制，让锁在作用域内全自动化
{
    public:
        explicit MutexLockGuard(MutexLock &_mutex)
            :mutex(_mutex)
        {
            mutex.lock();
        }
        ~MutexLockGuard()
        {
            mutex.unlock();
        }
    private:
        MutexLock &mutex;
        pid_t holder_;
};

#endif
