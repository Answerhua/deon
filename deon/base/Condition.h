#ifndef LOG_CONDITION_H
#define LOG_CONDITION_H

#include "MutexLock.h"
#include <pthread.h>
#include <errno.h>
#include <time.h>

class Condition : noncopyable
{
    public:
        explicit Condition(MutexLock &mutex):
            mutex_(mutex)
        {
            MCHECK(pthread_cond_init(&pcond_, NULL));
        }
        ~Condition()
        {
            MCHECK(pthread_cond_destroy(&pcond_));
        }
        void wait()
        {
            MCHECK(pthread_cond_wait(&pcond_, mutex_.get()));
        }
        void notify()
        {
            MCHECK(pthread_cond_signal(&pcond_));
        }
        void notifyAll()
        {
            MCHECK(pthread_cond_broadcast(&pcond_));
        }
        bool waitForSeconds(int seconds)
        {
            struct timespec abstime;
            clock_gettime(CLOCK_REALTIME, &abstime);
            abstime.tv_sec += static_cast<time_t>(seconds);
            return ETIMEDOUT == pthread_cond_timedwait(&pcond_, mutex_.get(), &abstime);
        }
    private:
        MutexLock &mutex_;
        pthread_cond_t pcond_;
};

#endif
