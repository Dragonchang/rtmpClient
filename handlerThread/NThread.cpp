#include"NThread.h"
#define DEBUG 0
NThread::NThread(){
    int res = sem_init(&bin_sem,0,0);
    mLooper = new Looper();
    running = false;
    if (DEBUG) printf("tid:%d NThread::NThread\n", (unsigned)pthread_self());
    pthread_create(&mTid,0,ThreadLoop,this);
    sem_wait(&bin_sem);
}

NThread::~NThread(){
    if(mLooper != NULL) {
        mLooper->quit(true);
        sem_wait(&bin_sem);
        delete mLooper;
        mLooper = NULL;
    }
    sem_destroy(&bin_sem);
}

void* NThread::ThreadLoop(void* arg){
    if (DEBUG) printf("tid:%d NThread::ThreadLoop\n", (unsigned)pthread_self());
    NThread *ptr = (NThread *)arg;
    ptr->Run();
}

void NThread::Run(){
    sem_post(&bin_sem);
    running = true;
    getLooper()->loop();
    running = false;
    sem_post(&bin_sem);
}
