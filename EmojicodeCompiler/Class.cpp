//
//  Class.c
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeCompiler.h"

std::map<std::array<EmojicodeChar, 2>, Class*> classesRegister;
std::map<std::array<EmojicodeChar, 2>, Protocol*> protocolsRegister;
std::map<std::array<EmojicodeChar, 2>, Enum*> enumsRegister;

Class* getClass(EmojicodeChar name, EmojicodeChar enamespace){
    std::array<EmojicodeChar, 2> ns = {enamespace, name};
    Class *cl = classesRegister.find(ns)->second;
    return cl;
}

bool Class::conformsTo(Protocol *to){
    for(Class *a = this->superclass; a != NULL; a = a->superclass){
        for(size_t i = 0; i < this->protocols.size(); i++){
            if(a->protocols[i] == to) {
                return true;
            }
        }
    }
    return false;
}

bool Class::inheritsFrom(Class *from){
    for(Class *a = this->superclass; a != NULL; a = a->superclass){
        if(a == from) {
            return true;
        }
    }
    return false;
}

Initializer* Class::getInitializer(EmojicodeChar name){
    for(auto eclass = this; eclass != NULL; eclass = eclass->superclass){
        auto pos = eclass->initializers.find(name);
        if(pos != eclass->initializers.end()){
            return pos->second;
        }
        if(!eclass->inheritsContructors){ //Does this eclass inherit initializers?
            break;
        }
    }
    return NULL;
}

Method* Class::getMethod(EmojicodeChar name){
    for(auto eclass = this; eclass != NULL; eclass = eclass->superclass){
        auto pos = eclass->methods.find(name);
        if(pos != eclass->methods.end()){
            return pos->second;
        }
    }
    return NULL;
}

ClassMethod* Class::getClassMethod(EmojicodeChar name){
    for(auto eclass = this; eclass != NULL; eclass = eclass->superclass){
        auto pos = eclass->classMethods.find(name);
        if(pos != eclass->classMethods.end()){
            return pos->second;
        }
    }
    return NULL;
}

void addProtocol(Class *c, Protocol *protocol){
    c->protocols.push_back(protocol);
}


//MARK: Protocol

Protocol* getProtocol(EmojicodeChar name, EmojicodeChar enamespace){
    std::array<EmojicodeChar, 2> ns = {enamespace, name};
    return protocolsRegister.find(ns)->second;
}

Method* protocolGetMethod(EmojicodeChar name, Protocol *protocol){
    return protocol->methods.find(name)->second;
}
