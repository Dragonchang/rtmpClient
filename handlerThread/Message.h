#ifndef H_MESSAGE
#define H_MESSAGE
#include"Handler.h"
#include<stdio.h>
class Handler;
class Message {
private: 
    //构造函数为私有的方法防止其它对象中创建该实例
    Message(int what) : what(what) {}
    Message(int what, void *obj) : what(what),mObj(obj) {}
public:
    virtual ~Message();
    Message(){mNext = NULL;mTarget = NULL;}
    static Message* obtain(int what);
    static Message* obtain(int what, void *obj);
public:
    void setTarget(Handler* handler);
    long long when;
    int what;
    Handler *mTarget;
    Message* mBefore;
    Message* mNext;
    void *mObj;
};
#endif
