#include "Looper.h"
#include "Utils.h"
static pthread_key_t gTLSKey = 0;
Looper::Looper() {
    mQuit = false;
    mMessageQueue = new MessageQueue();
    int result = pthread_key_create(& gTLSKey, NULL);
}

Looper::~Looper() {
   if (mMessageQueue != NULL) {
      delete mMessageQueue;
      mMessageQueue = NULL;
   }
}

void Looper::setForThread(Looper* looper){
    pthread_setspecific(gTLSKey, looper);
}

Looper* Looper::getForThread(){
    return (Looper*)pthread_getspecific(gTLSKey);
}

void Looper::loop() {
    Message * message;
    Looper::setForThread(this);
    for (;;) {
        if(DEBUG) printf("tid:%d Looper::loop \n",(unsigned)pthread_self());
        message = mMessageQueue->getNextMessage();
        if (message != NULL) {
            if(DEBUG) printf("tid:%d Looper::loop message != NULL\n",(unsigned)pthread_self());
            message->mTarget->handlerMessage(message);
            delete message;
        } else {
            if(DEBUG) printf("thread tid:%d exit!!!!\n",(unsigned)pthread_self());
            break;
        }
    }
}

void Looper::enqueueMessage(Message* message) {
    if (mQuit == true) {
        if(DEBUG) printf("tid:%d Looper is quit stop enqueue message\n", (unsigned)pthread_self());
        return;
    }
    if (mMessageQueue) {
        mMessageQueue->enqueueMessage(message);
    }
}

void Looper::quit(bool removeAllMessage) {
    if (mQuit == true) {
        if(DEBUG) printf("tid:%d Looper is quit stop quit\n", (unsigned)pthread_self());
        return;
    }
    mQuit = true;
    if (removeAllMessage) {
        mMessageQueue->removeAndDeleteAllMessage();
    }
    mMessageQueue->setQuit(true);
    mMessageQueue->wake();
}
