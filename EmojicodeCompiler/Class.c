//
//  Class.c
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeCompiler.h"

bool conformsTo(Class *a, Protocol *to){
    for(; a != NULL; a = a->superclass){
        for(size_t i = 0; i < a->protocols->count; i++){
            if(getList(a->protocols, i) == to) {
                return true;
            }
        }
    }
    return false;
}

bool inheritsFrom(Class *a, Class *from){
    for(; a != NULL; a = a->superclass){
        if(a == from) {
            return true;
        }
    }
    return false;
}

void releaseArgumentsStructure(Arguments *args){
    free(args->variables);
    free(args);
}


//MARK: Class

Dictionary *classesRegister;
Dictionary *protocolsRegister;
Dictionary *enumsRegister;

void initTypes(){
    classesRegister = newDictionary();
    protocolsRegister = newDictionary();
    enumsRegister = newDictionary();
}

Class* getClass(EmojicodeChar name, EmojicodeChar namespace){
    EmojicodeChar ns[2] = {namespace, name};
    Class *cl = dictionaryLookup(classesRegister, &ns, sizeof(ns));
    return cl;
}

Initializer* getInitializer(EmojicodeChar name, Class *class){
    for(; class != NULL; class = class->superclass){
        Initializer *c = dictionaryLookup(class->initializers, &name, sizeof(name));
        if(c != NULL){
            return c;
        }
        if(!class->inheritsContructors){ //Does this class inherit initializers?
            break;
        }
    }
    return NULL;
}

Method* getMethod(EmojicodeChar name, Class *class){
    for(; class != NULL; class = class->superclass){
        Method *m = dictionaryLookup(class->methods, &name, sizeof(name));
        if(m != NULL){
            return m;
        }
    }
    return NULL;
}

ClassMethod* getClassMethod(EmojicodeChar name, Class *class){
    for(; class != NULL; class = class->superclass){
        ClassMethod *m = dictionaryLookup(class->classMethods, &name, sizeof(name));
        if(m != NULL){
            return m;
        }
    }
    return NULL;
}

void addProtocol(Class *c, Protocol *protocol){
    appendList(c->protocols, protocol);
}


//MARK: Protocol

Protocol* newProtocol(EmojicodeChar name, EmojicodeChar namespace){
    Protocol *protocol = malloc(sizeof(Protocol));
    
    protocol->name = name;
    protocol->namespace = namespace;
    protocol->methodList = newList();
    protocol->methods = newDictionary();
    
    EmojicodeChar ns[2] = {namespace, name};
    dictionarySet(protocolsRegister, &ns, sizeof(ns), protocol);
    
    return protocol;
}

Method* protocolAddMethod(EmojicodeChar name, Protocol *protocol, Arguments arguments, Type returnType) {
    Method *method = malloc(sizeof(Method));
    method->pc.name = name;
    method->pc.arguments = arguments;
    method->pc.native = true;
    method->pc.access = PUBLIC;
    method->pc.final = false;
    method->pc.vti = protocol->methodList->count;
    method->pc.returnType = returnType;
    
    appendList(protocol->methodList, method);
    dictionarySet(protocol->methods, &name, sizeof(name), method);
    
    return method;
}

Protocol* getProtocol(EmojicodeChar name, EmojicodeChar namespace){
    EmojicodeChar ns[2] = {namespace, name};
    return dictionaryLookup(protocolsRegister, &ns, sizeof(ns));
}

Method* protocolGetMethod(EmojicodeChar name, Protocol *protocol){
    return dictionaryLookup(protocol->methods, &name, sizeof(name));
}


//MARK: Enums

Enum* newEnum(EmojicodeChar name, EmojicodeChar namespace){
    Enum *eenum = malloc(sizeof(Enum));
    
    eenum->name = name;
    eenum->dictionary = newDictionary();
    
    EmojicodeChar ns[2] = {namespace, name};
    dictionarySet(enumsRegister, &ns, sizeof(ns), eenum);
    
    return eenum;
}

void enumAddValue(EmojicodeChar name, Enum *eenum, EmojicodeInteger value) {
    EmojicodeInteger *im = malloc(sizeof(EmojicodeInteger));
    *im = value;
    dictionarySet(eenum->dictionary, &name, sizeof(name), im);
}

Enum* getEnum(EmojicodeChar name, EmojicodeChar namespace){
    EmojicodeChar ns[2] = {namespace, name};
    return dictionaryLookup(enumsRegister, &ns, sizeof(ns));
}

EmojicodeInteger* enumGetValue(EmojicodeChar name, Enum *eenum){
    return dictionaryLookup(eenum->dictionary, &name, sizeof(name));
}