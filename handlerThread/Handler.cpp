#include"Handler.h"
#include "Utils.h"
Handler::Handler(Looper* looper){
    mLooper = looper;
}

Handler::~Handler(){

}

void Handler::sendMessage(Message *message){
    if (message != NULL) {
        message->setTarget(this);
        message->when = getCurrentTime();
        if (DEBUG) printf("tid:%d Handler::sendMessage what =%d message->when = %lld\n",(unsigned)pthread_self() ,message->what,message->when);
    } else {
        printf("Handler::sendMessage message is null\n");
        return;
    }
    if (mLooper != NULL) {
        mLooper->enqueueMessage(message);
    } else {
        printf("looper is null\n");
    }
}

void Handler::sendMessageDelayed(Message* message, long delayMillis) {
    if (message != NULL) {
        message->setTarget(this);
        message->when = getCurrentTime() + delayMillis;
        if (DEBUG) printf("tid:%d Handler::sendMessageDelayed what =%d message->when = %lld delayMillis =%ld \n",(unsigned)pthread_self() ,message->what, message->when, delayMillis);
    } else {
        printf("Handler::sendMessage message is null\n");
        return;
    }
    if (mLooper != NULL) {
        mLooper->enqueueMessage(message);
    } else {
        printf("looper is null\n");
    }
}

void Handler::handlerMessage(Message *message){
    if (DEBUG) printf("tid:%d Handler::handlerMessage what =%d\n",(unsigned)pthread_self() ,message->what);
    sleep(2);
}
