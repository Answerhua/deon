#include "Thread.h"
#include "CurrentThread.h"
#include "Logging.h"
#include <string>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <assert.h>

namespace CurrentThread
{
    __thread int t_cachedTid = 0;
    __thread char t_tidString[32];
    __thread int t_tidStringLength = 6;
    __thread const char* t_threadName = "default";
}

pid_t gettid()
{
    return static_cast<pid_t>(::syscall(SYS_gettid));   //获取真实线程id的唯一标识
}

void CurrentThread::cacheTid()
{
    if (t_cachedTid == 0)
    {
        t_cachedTid = gettid();     
        t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid); //将真实线程id写进字符变量中
    }
}

struct ThreadData  //线程数据类，观察者模式
{
    typedef Thread::ThreadFunc ThreadFunc;
    ThreadFunc func_;
    std::string name_;
    pid_t* tid_;
    CountDownLatch* latch_;

    ThreadData(const ThreadFunc &func, const std::string& name, pid_t *tid, CountDownLatch *latch)
    :   func_(func),
        name_(name),
        tid_(tid),
        latch_(latch)
    { }

    void runInThread() //线程运行
    {
        *tid_ = CurrentThread::tid(); //得到线程id
        tid_ = NULL;
        latch_->countDown();
        latch_ = NULL;

        CurrentThread::t_threadName = name_.empty() ? "Thread" : name_.c_str();
        prctl(PR_SET_NAME, CurrentThread::t_threadName); //设置名字

        func_(); //调用方法
        CurrentThread::t_threadName = "finished";
    }
};

void *startThread(void* obj) 
{
    ThreadData* data = static_cast<ThreadData*>(obj);
    data->runInThread(); 
    delete data;
    return NULL;
}


Thread::Thread(const ThreadFunc &func, const std::string &n)
  : started_(false),
    joined_(false),
    pthreadId_(0),
    tid_(0),
    func_(func),
    name_(n),
    latch_(1)
{
    setDefaultName();
}

Thread::~Thread()
{
    if (started_ && !joined_)
        pthread_detach(pthreadId_);
}

void Thread::setDefaultName()
{
    if (name_.empty())
    {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread");
        name_ = buf;
    }
}

void Thread::start()
{
    assert(!started_); //确保线程没有启动
    started_ = true;   //设置标志，表示线程已经启动
    ThreadData* data = new ThreadData(func_, name_, &tid_, &latch_);//data存放线程真正要执行的函数func，线程id，线程name等信息
    if (pthread_create(&pthreadId_, NULL, &startThread, data))  //创建线程
    {
        started_ = false;  //创建线程失败
        delete data;
        LOG << "Failed in pthread_create";
    }
    else
    {
        latch_.wait();
        assert(tid_ > 0);
    }
}

int Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadId_, NULL);
}

