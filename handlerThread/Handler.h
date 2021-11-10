#ifndef H_HANDLER
#define H_HANDLER
#include"Message.h"
#include"Looper.h"
#include<stdio.h>
#include<pthread.h>
class Message;
class Looper;
class Handler {
public:
    Handler(){}
    Handler(Looper* looper);
    virtual ~Handler();

protected:
    Looper *mLooper;

public:
    virtual void handlerMessage(Message* message);
    void sendMessage(Message* message);
    void sendMessageDelayed(Message* message, long delayMillis);
};
#endif
