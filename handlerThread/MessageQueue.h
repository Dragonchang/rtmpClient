#ifndef H_MESSAGEQUEUE
#define H_MESSAGEQUEUE
#include "Message.h"
#include <fcntl.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "Meutex.h"
#define USE_PIPE
/*
保存消息实体,looper会在该对象上等待消息进行处理;
looper会发送相应的消息到该对象中.
++++++++++++++++++++++++++++++++++++++
+ 1.链表
+ 2.waite queue
++++++++++++++++++++++++++++++++++++++
需要优化message使用排序二叉树来保存
*/
class Message;
class MessageQueue {
public:
    MessageQueue();
    virtual ~MessageQueue();
    void queueAtFront(Message* message);
    void removeAndDeleteAllMessage();


    bool isEmpty() {return mQueueSize == 0;}
    void setQuit(bool quit) { mQuit = quit;}
    int pollOnce(int timeoutMillis); //主线程在messagequeue中等待消息事件时的睡眠函数
    void wake(); //消息发送到messageueue的wake函数
    void enqueueMessage(Message* message);
    Message *getNextMessage();

private:
    Message* getMessage(long long currentTime);
    void removeMessage(Message* message);

private:
    Message* mHead;
    Message* mTail;
    Message* mCurrentMessage;
    int mQueueSize;
    bool mQuit;
#ifdef USE_PIPE
    int mWakeReadPipeFd;  // immutable
    int mWakeWritePipeFd; // immutable
#else
    int mWakeEventFd; //消息发送时唤醒fd
#endif
    int mEpollFd;  //主线程等待消息时waite的fd
    void awoken();
    void buildEpollLocked();
    Mutex mMutex;

};
#endif
