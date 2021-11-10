#include"MessageQueue.h"
#include "Utils.h"
static const int EPOLL_SIZE_HINT = 1024;
// Maximum number of file descriptors for which to retrieve poll events each iteration.
static const int EPOLL_MAX_EVENTS = 16;
MessageQueue::MessageQueue():mEpollFd(-1) {
    mHead = new Message();
    mTail = new Message();
    mCurrentMessage = NULL;
    mHead->mNext = mTail;
    mHead->mBefore = mTail;
    mTail->mNext = mHead;
    mTail->mBefore = mHead;
    mQueueSize = 0;
    buildEpollLocked();
    mQuit = false;
}

void MessageQueue::buildEpollLocked() {
    if (mEpollFd >= 0) {
        close(mEpollFd);
    }
    int result = 0;

#ifdef USE_PIPE
    int wakeFds[2];
    result= pipe(wakeFds);
    if(result !=0) {
        if (DEBUG) printf("tid:%d MessageQueue::buildEpollLocked pipe failed\n",(unsigned)pthread_self());
    }  
    mWakeReadPipeFd = wakeFds[0];
    mWakeWritePipeFd = wakeFds[1];
    result = fcntl(mWakeReadPipeFd, F_SETFL, O_NONBLOCK);
    if(result !=0) {
        if (DEBUG) printf("tid:%d MessageQueue::buildEpollLocked fcntl mWakeReadPipeFd failed\n",(unsigned)pthread_self());
    }  
    result = fcntl(mWakeWritePipeFd, F_SETFL, O_NONBLOCK);
    if(result !=0) {
        if (DEBUG) printf("tid:%d MessageQueue::buildEpollLocked fcntl mWakeWritePipeFd failed\n",(unsigned)pthread_self());
    }  
#else
    mWakeEventFd = eventfd(0, EFD_NONBLOCK);
    if(mWakeEventFd <0) {
        if (DEBUG) printf("tid:%d MessageQueue::buildEpollLocked eventfd failed\n",(unsigned)pthread_self());
    }
#endif
    // Allocate the new epoll instance and register the wake pipe.
    mEpollFd = epoll_create(EPOLL_SIZE_HINT);
    if(mEpollFd < 0) {
        if (DEBUG) printf("tid:%d MessageQueue::buildEpollLocked epoll_create failed\n",(unsigned)pthread_self());
    }
    struct epoll_event eventItem;
    memset(& eventItem, 0, sizeof(epoll_event)); // zero out unused members of data field union
    eventItem.events = EPOLLIN;

#ifdef USE_PIPE
    eventItem.data.fd = mWakeReadPipeFd;
    result = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mWakeReadPipeFd, & eventItem);
#else
    eventItem.data.fd = mWakeEventFd;
    result = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mWakeEventFd, & eventItem);
#endif

    if(result != 0) {
        if (DEBUG) printf("tid:%d MessageQueue::buildEpollLocked epoll_ctl add failed\n",(unsigned)pthread_self());
    }
}

MessageQueue::~MessageQueue() {
   removeAndDeleteAllMessage();
   if (mHead != NULL) {
      delete mHead;
      mHead = NULL;
   }
   if (mTail != NULL) {
      delete mTail;
      mTail = NULL;
   }
#ifdef USE_PIPE
    close(mWakeReadPipeFd);
    close(mWakeWritePipeFd);
#else
    close(mWakeEventFd);
#endif
    if (mEpollFd >= 0) {
        close(mEpollFd);
    }
}

void MessageQueue::queueAtFront(Message* message) {
    if (DEBUG) printf("tid:%d MessageQueue::queueAtFront message%p\n",(unsigned)pthread_self(), message);
    Mutex::Autolock _l(mMutex);
    mHead->mNext->mBefore = message;
    message->mNext = mHead->mNext;
    mHead->mNext = message;
    message->mBefore = mHead;
    mCurrentMessage = message;
    mQueueSize = mQueueSize + 1;
    if (DEBUG) printf("tid:%d MessageQueue::queueAtFront mQueueSize = %d\n",(unsigned)pthread_self(), mQueueSize);
}

void MessageQueue::removeAndDeleteAllMessage() {
    Mutex::Autolock _l(mMutex);
    //TODO 需要遍历链表删除message
    Message* indexMessage;
    Message* removeMessage;
    if (mHead->mNext == mTail) {
        if (DEBUG) printf("tid:%d removeAndDeleteAllMessage:MessageQueue is empty!\n",(unsigned)pthread_self());
        return;
    }
    for(indexMessage = mHead->mNext; indexMessage != mTail;indexMessage = mHead->mNext) {
        mHead->mNext = indexMessage->mNext;
        indexMessage->mNext->mBefore = mHead;
        if(indexMessage != NULL) delete indexMessage;
        mQueueSize = mQueueSize - 1;
        if (DEBUG) printf("tid:%d removeAndDeleteAllMessage:mQueueSize =%d!\n",(unsigned)pthread_self(), mQueueSize);
    }
}

int MessageQueue::pollOnce(int timeoutMillis) {
    int result = 0;
    for (;;) {
        if (DEBUG) printf("tid:%d MessageQueue::pollOnce result = %d.\n",(unsigned)pthread_self(), result);
        if (result != 0) {
            if (DEBUG) printf("tid:%d MessageQueue::pollOnce result != 0.\n",(unsigned)pthread_self());
            break;
        }
        struct epoll_event eventItems[EPOLL_MAX_EVENTS];
        result = -1;
        if (DEBUG) printf("tid:%d MessageQueue::pollOnce epoll_wait, begin\n", (unsigned)pthread_self());
        int eventCount = epoll_wait(mEpollFd, eventItems, EPOLL_MAX_EVENTS, timeoutMillis);
        if (DEBUG) printf("tid:%d MessageQueue::pollOnce epoll_wait, end eventCount=%d\n", (unsigned)pthread_self() ,eventCount);
        // Check for poll error.
        if (eventCount < 0) {
	    if (errno == EINTR) {
	        return result;
	    }
            if (DEBUG) printf("ERROR tid:%d MessageQueue::pollOnce Poll failed with an unexpected error.\n",(unsigned)pthread_self());
	    result = -2;
	    return result;
        }

        // Check for poll timeout.
        if (eventCount == 0) {
            if (DEBUG) printf("tid:%d MessageQueue::pollOnce timeout.\n",(unsigned)pthread_self());
	    result = -3;
	    return result;
        }
        for (int i = 0; i < eventCount; i++) {
            int fd = eventItems[i].data.fd;
            uint32_t epollEvents = eventItems[i].events;
#ifdef USE_PIPE
            if (fd ==mWakeReadPipeFd){
#else
            if (fd == mWakeEventFd) {
#endif
                if (epollEvents & EPOLLIN) {
                    awoken();//当不调用该函数在有event的fd上将数据读出,那么epoll_wait会一直返回.
                } else {
                    if (DEBUG) printf("tid:%d Ignoring unexpected epoll events 0x%x on wake event fd.\n",(unsigned)pthread_self() ,epollEvents);
                }
            }
        }
    }
    return result;
}

void MessageQueue::awoken() {
#ifdef USE_PIPE
    char buffer[1];
    ssize_t nRead;
    //do {
        nRead = read(mWakeReadPipeFd, buffer, sizeof(buffer));
    //} while ((nRead == -1 && errno == EINTR) || nRead == sizeof(buffer));
    if (DEBUG) printf("tid:%d awoken buffer = %s nRead = %zu\n", (unsigned)pthread_self(),buffer,nRead);
#else
    uint64_t counter;
    ssize_t nRead = read(mWakeEventFd, &counter, 8);//sizeof(uint64_t));
    if (DEBUG) printf("tid:%d awoken counter = %llu nRead = %zu\n", (unsigned)pthread_self(),counter,nRead);
    if (nRead != sizeof(uint64_t)) {
        //if (errno != EAGAIN) {
            printf("tid:%d Could not read \n",(unsigned)pthread_self());
        //}
    }
#endif
}

void MessageQueue::wake() {
#ifdef USE_PIPE
    ssize_t nWrite;
    do {
        nWrite = write(mWakeWritePipeFd, "W", 1);
    } while (nWrite == -1 && errno == EINTR);
    if (DEBUG) printf("tid:%d wake  nWrite = %zu\n", (unsigned)pthread_self(),nWrite);
    if (nWrite != 1) {
        if (DEBUG) printf("tid:%d Could not write wake signal\n",(unsigned)pthread_self());
    }
#else
    uint64_t inc = 1;
    ssize_t nWrite = write(mWakeEventFd, &inc, sizeof(uint64_t));
    if (DEBUG) printf("tid:%d wake inc = %llu nWrite = %zu\n", (unsigned)pthread_self(),inc,nWrite);
    if (nWrite != sizeof(uint64_t)) {
        //if (errno != EAGAIN) {
            printf("tid:%d Could not write wake signal\n",(unsigned)pthread_self());
        //}
    }
#endif
}

void MessageQueue::enqueueMessage(Message* message){
    if (DEBUG) printf("tid:%d MessageQueue::enqueueMessage! message = %p\n",(unsigned)pthread_self(), message);
    queueAtFront(message);
    wake();
}

Message* MessageQueue::getMessage(long long currentTime) {
    long tempDelayTime = 0;
    Message* indexMessage = NULL;
    if(isEmpty()) {
        if (DEBUG) printf("ERROR tid:%d MessageQueue::getMessage isEmpty mQueueSize =%d \n",(unsigned)pthread_self(), mQueueSize);
        return NULL;
    }
    Message* tempMessage = mTail->mBefore;
    long minDelayTime = tempMessage->when - currentTime;
    for(indexMessage = tempMessage; indexMessage != mHead;indexMessage = indexMessage->mBefore) {
        tempDelayTime = indexMessage->when - currentTime;
        if (tempDelayTime < minDelayTime) {
            minDelayTime = tempDelayTime;
            tempMessage = indexMessage;
        }
    }
    return tempMessage;
}

void MessageQueue::removeMessage(Message* message) {
    Message* indexMessage;
    for(indexMessage = mHead->mNext; indexMessage != mTail;indexMessage = indexMessage->mNext) {
       if (indexMessage == message) {
           if (DEBUG) printf("tid:%d MessageQueue::removeMessage find the message = %p and remove it!\n",(unsigned)pthread_self(), message);
           indexMessage->mBefore->mNext = indexMessage->mNext;
           indexMessage->mNext->mBefore = indexMessage->mBefore;
           mQueueSize = mQueueSize - 1;
           if (DEBUG) printf("tid:%d MessageQueue::removeMessage mQueueSize =%d!\n",(unsigned)pthread_self(), mQueueSize);
           break;
       }
    }
}


Message* MessageQueue::getNextMessage(){
    long nextPollTimeoutMillis = 0;
    for (;;) {
        if (DEBUG) printf("tid:%d MessageQueue::getNextMessage nextPollTimeoutMillis = %ld\n",(unsigned)pthread_self(), nextPollTimeoutMillis);
        pollOnce(nextPollTimeoutMillis);
        {
            Mutex::Autolock _l(mMutex);
            Message* message = NULL;
            long long currentTime = getCurrentTime();
            if (!isEmpty()) {
                message = getMessage(currentTime);
                if (message != NULL) {
                    if (message->when <= currentTime) {
                        removeMessage(message);
                        return message;
                    } else {
                        nextPollTimeoutMillis = message->when - currentTime;
                        if (DEBUG) printf("tid:%d get a delay Message delay time = %ld\n",(unsigned)pthread_self(), nextPollTimeoutMillis);
                        continue;
                    }
                } else {
                    if (DEBUG) printf("ERROR tid:%d MessageQueue::getNextMessage isEmpty thread exit!\n",(unsigned)pthread_self());
                    return NULL;
                }
            } else {
                if (DEBUG) printf("tid:%d MessageQueue::getNextMessage isEmpty!\n",(unsigned)pthread_self());
                if (mQuit) {
                    if (DEBUG) printf("tid:%d MessageQueue::getNextMessage isEmpty thread exit!\n",(unsigned)pthread_self());
                    return NULL;
                } else {
                    if (DEBUG) printf("tid:%d MessageQueue::getNextMessage isEmpty and enter poll_wait new message!\n",(unsigned)pthread_self());
                    nextPollTimeoutMillis = -1;
                }
            }
        }
    }
}


























