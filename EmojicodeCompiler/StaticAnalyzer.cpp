//
//  StaticAnalyzer.c
//  Emojicode
//
//  Created by Theo Weidmann on 05.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeCompiler.hpp"
#include "StaticAnalyzer.hpp"
#include "StaticFunctionAnalyzer.hpp"
#include "Writer.hpp"
#include "Lexer.hpp"
#include "CompilerScope.hpp"
#include "Class.hpp"
#include "utf8.h"
#include <cstring>

void analyzeClass(Type classType, Writer &writer){
    auto eclass = classType.eclass;
    
    writer.writeEmojicodeChar(eclass->name);
    if(eclass->superclass){
        writer.writeUInt16(eclass->superclass->index);
    }
    else { //If the eclass does not have a superclass the own index gets written
        writer.writeUInt16(eclass->index);
    }
    
    Scoper scoper;
    Scope objectScope(true);
    
    //Get the ID offset for this eclass by summing up all superclasses instance variable counts
    uint16_t offset = 0;
    for(Class *aClass = eclass->superclass; aClass != nullptr; aClass = aClass->superclass){
        offset += aClass->instanceVariables.size();
    }
    writer.writeUInt16(eclass->instanceVariables.size() + offset);
    
    //Number of methods inclusive superclass
    writer.writeUInt16(eclass->nextMethodVti);
    //Number of eclass methods inclusive superclass
    writer.writeUInt16(eclass->nextClassMethodVti);
    //Initializer inclusive superclass
    writer.writeByte(eclass->inheritsContructors ? 1 : 0);
    writer.writeUInt16(eclass->nextInitializerVti);
    
    writer.writeUInt16(eclass->methodList.size());
    writer.writeUInt16(eclass->initializerList.size());
    writer.writeUInt16(eclass->classMethodList.size());
    
    for (auto var : eclass->instanceVariables) {
        CompilerVariable *cv = new CompilerVariable(var->type, offset++, 1, false);
        cv->variable = var;
        objectScope.setLocalVariable(var->name, cv);
    }
    
    scoper.pushScope(&objectScope);
    
    for (auto method : eclass->methodList) {
        StaticFunctionAnalyzer::writeAndAnalyzeProcedure(*method, writer, classType, scoper);
    }
    
    for (auto initializer : eclass->initializerList) {
        StaticFunctionAnalyzer::writeAndAnalyzeProcedure(*initializer, writer, classType, scoper, false, initializer);
    }
    
    scoper.popScope();
    
    for (auto classMethod : eclass->classMethodList) {
        StaticFunctionAnalyzer::writeAndAnalyzeProcedure(*classMethod, writer, classType, scoper, true);
    }
    
    if (eclass->instanceVariables.size() > 0 && eclass->initializerList.size() == 0) {
        auto str = classType.toString(typeNothingness, true);
        compilerWarning(eclass->classBegin, "Class %s defines %d instances variables but has no initializers.", str.c_str(), eclass->instanceVariables.size());
    }
    
    writer.writeUInt16(eclass->protocols().size());
    
    if (eclass->protocols().size() > 0) {
        auto biggestPlaceholder = writer.writePlaceholder<uint16_t>();
        auto smallestPlaceholder = writer.writePlaceholder<uint16_t>();

        uint_fast16_t smallestProtocolIndex = UINT_FAST16_MAX;
        uint_fast16_t biggestProtocolIndex = 0;
        
        for (auto protocol : eclass->protocols()) {
            writer.writeUInt16(protocol->index);
            
            if (protocol->index > biggestProtocolIndex) {
                biggestProtocolIndex = protocol->index;
            }
            if (protocol->index < smallestProtocolIndex) {
                smallestProtocolIndex = protocol->index;
            }
            
            writer.writeUInt16(protocol->methods().size());
            
            for (auto method : protocol->methods()) {
                Method *clm = eclass->getMethod(method->name);
                
                if (clm == nullptr) {
                    auto className = classType.toString(typeNothingness, true);
                    auto protocolName = Type(protocol, false).toString(typeNothingness, true);
                    ecCharToCharStack(method->name, ms);
                    compilerError(eclass->classBegin, "Class %s does not agree to protocol %s: Method %s is missing.", className.c_str(), protocolName.c_str(), ms);
                }
                
                writer.writeUInt16(clm->vti);
                clm->checkPromises(method, "protocol definition", classType);
            }
        }
        
        biggestPlaceholder.write(biggestProtocolIndex);
        smallestPlaceholder.write(smallestProtocolIndex);
    }
}

void analyzeClassesAndWrite(FILE *fout){
    Writer writer(fout);
    
    stringPool.push_back(new Token());
    
    writer.writeByte(ByteCodeSpecificationVersion);
    
    //Decide which classes inherit initializers, if they agree to protocols, and assign virtual table indexes before we analyze the classes!
    for (auto eclass : classes) {
        //decide whether this eclass is eligible for initializer inheritance
        if(eclass->instanceVariables.size() == 0 && eclass->initializerList.size() == 0){
            eclass->inheritsContructors = true;
        }
        
        if(eclass->superclass){
            eclass->nextClassMethodVti = eclass->superclass->nextClassMethodVti;
            eclass->nextInitializerVti = eclass->inheritsContructors ? eclass->superclass->nextInitializerVti : 0;
            eclass->nextMethodVti = eclass->superclass->nextMethodVti;
        }
        else {
            eclass->nextClassMethodVti = 0;
            eclass->nextInitializerVti = 0;
            eclass->nextMethodVti = 0;
        }
        
        Type classType = Type(eclass);
        
        for(auto method : eclass->methodList){
            Method *superMethod = eclass->superclass->getMethod(method->name);

            method->checkOverride(superMethod);
            if (superMethod){
                method->checkPromises(superMethod, "super method", classType);
                method->vti = superMethod->vti;
            }
            else {
                method->vti = eclass->nextMethodVti++;
            }
        }
        for(auto clMethod : eclass->classMethodList){
            ClassMethod *superMethod = eclass->superclass->getClassMethod(clMethod->name);
            
            clMethod->checkOverride(superMethod);
            if (superMethod){
                clMethod->checkPromises(superMethod, "super classmethod", classType);
                clMethod->vti = superMethod->vti;
            }
            else {
                clMethod->vti = eclass->nextClassMethodVti++;
            }
        }
        
        auto subRequiredInitializerNextVti = eclass->superclass ? eclass->superclass->requiredInitializerList.size() : 0;
        eclass->nextInitializerVti += eclass->requiredInitializerList.size();
        for(auto initializer : eclass->initializerList){
            Initializer *superInit = eclass->superclass->getInitializer(initializer->name);
            
            initializer->checkOverride(superInit);
            
            if (initializer->required) {
                if (superInit) {
                    initializer->checkPromises(superInit, "super classmethod", classType);
                    initializer->vti = superInit->vti;
                }
                else {
                    initializer->vti = subRequiredInitializerNextVti++;
                }
            }
            else {
                initializer->vti = eclass->nextInitializerVti++;
            }
        }
    }
    
    writer.writeUInt16(classes.size());
    
    uint8_t pkgCount = (uint8_t)packages.size();
    //must be s and _
    if(pkgCount == 2){
        pkgCount = 1;
    }
    if(pkgCount > 253){
        compilerError(nullptr, "You exceeded the maximum of 253 packages.");
    }
    writer.writeByte(pkgCount);
    
    size_t pkgI = 0;
    
    Package *pkg = nullptr;
    Class *eclass;
    for (size_t i = 0; eclass = classes[i], i < classes.size(); i++) {
        if((pkg != eclass->package && pkgCount > 1) || !pkg){ //pkgCount > 1: Ignore the second s
            if (i > 0){
                writer.writeByte(0);
            }
            pkg = eclass->package;
            Package *pkg = packages[pkgI++];
            
            uint16_t l = strlen(pkg->name) + 1;
            writer.writeUInt16(l);
            
            writer.writeBytes(pkg->name, l);
            
            writer.writeUInt16(pkg->version.major);
            writer.writeUInt16(pkg->version.minor);

            writer.writeByte(pkg->requiresNativeBinary ? 1 : 0);
        }
        else if (i > 0){
            writer.writeByte(1);
        }
        
        analyzeClass(Type(eclass), writer);
    }
    writer.writeByte(0);
    
    writer.writeUInt16(stringPool.size());
    for (auto token : stringPool) {
        writer.writeUInt16(token->value.size());
        
        for (auto c : token->value) {
            writer.writeEmojicodeChar(c);
        }
    }
    
    writer.writeUInt16(startingFlag.eclass->index);
    writer.writeUInt16(startingFlag.eclass->getClassMethod(E_CHEQUERED_FLAG)->vti);
}