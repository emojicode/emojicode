//
//  Class.c
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeCompiler.h"

Class* getClass(EmojicodeChar name, EmojicodeChar enamespace){
    std::array<EmojicodeChar, 2> ns = {enamespace, name};
    auto it = classesRegister.find(ns);
    return it != classesRegister.end() ? it->second : NULL;
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
    for(Class *a = this; a != NULL; a = a->superclass){
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
    auto it = protocolsRegister.find(ns);
    return it != protocolsRegister.end() ? it->second : NULL;
}

Method* Protocol::getMethod(EmojicodeChar name){
    auto it = methods.find(name);
    return it != methods.end() ? it->second : NULL;
}

//MARK: Enum

Enum* getEnum(EmojicodeChar name, EmojicodeChar enamespace){
    std::array<EmojicodeChar, 2> ns = {enamespace, name};
    auto it = enumsRegister.find(ns);
    return it != enumsRegister.end() ? it->second : NULL;
}

std::pair<bool, EmojicodeInteger> Enum::getValueFor(EmojicodeChar c) const {
    auto it = map.find(c);
    if (it == map.end()) {
        return std::pair<bool, EmojicodeInteger>(false, 0);
    }
    else {
        return std::pair<bool, EmojicodeInteger>(true, it->second);
    }
}

void Enum::addValueFor(EmojicodeChar c){
    map[c] = valuesCounter++;
}