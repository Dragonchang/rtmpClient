#ifndef H_NTHREAD
#define H_NTHREAD
#include"Looper.h"
#include<pthread.h>
#include<stdio.h>
#include <semaphore.h> 
class NThread {
public:
    NThread();
    virtual ~NThread();

protected:
    Looper* mLooper;
    pthread_t mTid;
    sem_t bin_sem;

public:
    static void* ThreadLoop(void* arg);
    Looper* getLooper() { return mLooper;}
    pthread_t getTid() {return mTid;}
    virtual void Run();
    bool IsRun() {return running;}
private:
    bool running;
};
#endif
