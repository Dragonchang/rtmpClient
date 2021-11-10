#include"Message.h"
Message::~Message(){
}

void Message::setTarget(Handler* handler) {
    mTarget = handler;
}

Message* Message::obtain(int what){
    return new Message(what);
}

Message* Message::obtain(int what, void *obj){
    return new Message(what, obj);
}
