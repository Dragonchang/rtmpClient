#ifndef H_LOOPER
#define H_LOOPER
#include<stdio.h>
#include "MessageQueue.h"
#include "Message.h"
#include <pthread.h> 
/*********************
1.一个线程对应一个looper对象
2.在looper所对应的线程中需要退出loop循环(TLS)
3.其它线程需要让loop退出循环 通过Nthread
*********************/
class MessageQueue;
class Message;
class Looper {
public:
    virtual ~Looper();

public:
    Looper();
    void loop();
    void quit(bool removeAllMessage);
    MessageQueue* getMessageQueue() { return mMessageQueue;}
    void enqueueMessage(Message* message);
    static void setForThread(Looper* looper);
    static Looper* getForThread();

private:
    MessageQueue* mMessageQueue;
    bool mQuit;
};
#endif
