//
//  Class.c
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeCompiler.h"

std::map<EmojicodeChar[2], Class*> classesRegister;
std::map<EmojicodeChar[2], Protocol*> protocolsRegister;
std::map<EmojicodeChar[2], Enum*> enumsRegister;

Class* getClass(EmojicodeChar name, EmojicodeChar enamespace){
    EmojicodeChar ns[2] = {enamespace, name};
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
    EmojicodeChar ns[2] = {enamespace, name};
    return protocolsRegister.find(ns)->second;
}

Method* protocolGetMethod(EmojicodeChar name, Protocol *protocol){
    return protocol->methods.find(name)->second;
}


//MARK: Enums

Enum* newEnum(EmojicodeChar name, EmojicodeChar enamespace){
    Enum *eenum = malloc(sizeof(Enum));
    
    eenum->name = name;
    eenum->dictionary = newDictionary();
    
    EmojicodeChar ns[2] = {enamespace, name};
    dictionarySet(enumsRegister, &ns, sizeof(ns), eenum);
    
    return eenum;
}

void enumAddValue(EmojicodeChar name, Enum *eenum, EmojicodeInteger value) {
    EmojicodeInteger *im = malloc(sizeof(EmojicodeInteger));
    *im = value;
    dictionarySet(eenum->dictionary, &name, sizeof(name), im);
}

Enum* getEnum(EmojicodeChar name, EmojicodeChar enamespace){
    EmojicodeChar ns[2] = {enamespace, name};
    return dictionaryLookup(enumsRegister, &ns, sizeof(ns));
}

EmojicodeInteger* enumGetValue(EmojicodeChar name, Enum *eenum){
    return dictionaryLookup(eenum->dictionary, &name, sizeof(name));
}