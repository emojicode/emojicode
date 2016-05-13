//
//  StaticAnalyzer.c
//  Emojicode
//
//  Created by Theo Weidmann on 05.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include <cstring>
#include "StaticAnalyzer.hpp"
#include "StaticFunctionAnalyzer.hpp"
#include "Writer.hpp"
#include "Protocol.hpp"
#include "CallableScoper.hpp"
#include "Class.hpp"
#include "utf8.h"
#include "EmojicodeCompiler.hpp"
#include "StringPool.hpp"

void analyzeClass(Type classType, Writer &writer) {
    auto eclass = classType.eclass;
    
    writer.writeEmojicodeChar(eclass->name());
    if (eclass->superclass) {
        writer.writeUInt16(eclass->superclass->index);
    }
    else {
        // If the class does not have a superclass the own index is written.
        writer.writeUInt16(eclass->index);
    }
    
    Scope objectScope;
    
    // Get the ID offset for this eclass by summing up all superclasses instance variable counts
    uint16_t offset = 0;
    for (Class *aClass = eclass->superclass; aClass != nullptr; aClass = aClass->superclass) {
        offset += aClass->instanceVariables().size();
    }
    auto instanceVariableCount = eclass->instanceVariables().size() + offset;
    writer.writeUInt16(instanceVariableCount);
    
    // Number of methods inclusive superclass
    writer.writeUInt16(eclass->nextMethodVti);
    // Number of eclass methods inclusive superclass
    writer.writeUInt16(eclass->nextClassMethodVti);
    // Initializer inclusive superclass
    writer.writeByte(eclass->inheritsContructors ? 1 : 0);
    writer.writeUInt16(eclass->nextInitializerVti);
    
    writer.writeUInt16(eclass->methodList.size());
    writer.writeUInt16(eclass->initializerList.size());
    writer.writeUInt16(eclass->classMethodList.size());
    
    for (auto &var : eclass->instanceVariables()) {
        objectScope.setLocalVariable(var.name.value, Variable(var.type, offset++, 1, false, var.name));
    }
    
    for (auto method : eclass->methodList) {
        auto scoper = CallableScoper(&objectScope);
        StaticFunctionAnalyzer::writeAndAnalyzeProcedure(method, writer, classType, scoper);
    }
    
    for (auto initializer : eclass->initializerList) {
        auto scoper = CallableScoper(&objectScope);
        StaticFunctionAnalyzer::writeAndAnalyzeProcedure(initializer, writer, classType, scoper, false,
                                                         initializer);
    }
    
    for (auto classMethod : eclass->classMethodList) {
        auto scoper = CallableScoper();
        StaticFunctionAnalyzer::writeAndAnalyzeProcedure(classMethod, writer, classType, scoper, true);
    }
    
    if (eclass->instanceVariables().size() > 0 && eclass->initializerList.size() == 0) {
        auto str = classType.toString(typeNothingness, true);
        compilerWarning(eclass->position(), "Class %s defines %d instances variables but has no initializers.",
                        str.c_str(), eclass->instanceVariables().size());
    }
    
    writer.writeUInt16(eclass->protocols().size());
    
    if (eclass->protocols().size() > 0) {
        auto biggestPlaceholder = writer.writePlaceholder<uint16_t>();
        auto smallestPlaceholder = writer.writePlaceholder<uint16_t>();

        uint_fast16_t smallestProtocolIndex = UINT_FAST16_MAX;
        uint_fast16_t biggestProtocolIndex = 0;
        
        for (auto protocol : eclass->protocols()) {
            writer.writeUInt16(protocol.protocol->index);
            
            if (protocol.protocol->index > biggestProtocolIndex) {
                biggestProtocolIndex = protocol.protocol->index;
            }
            if (protocol.protocol->index < smallestProtocolIndex) {
                smallestProtocolIndex = protocol.protocol->index;
            }
            
            writer.writeUInt16(protocol.protocol->methods().size());
            
            for (auto method : protocol.protocol->methods()) {
                try {
                    Method *clm = eclass->lookupMethod(method->name);
                    if (clm == nullptr) {
                        auto className = classType.toString(typeNothingness, true);
                        auto protocolName = protocol.toString(typeNothingness, true);
                        ecCharToCharStack(method->name, ms);
                        throw CompilerErrorException(eclass->position(),
                                      "Class %s does not agree to protocol %s: Method %s is missing.",
                                      className.c_str(), protocolName.c_str(), ms);
                    }
                    
                    writer.writeUInt16(clm->vti);
                    Procedure::checkReturnPromise(clm->returnType, method->returnType.resolveOn(protocol, false),
                                                  method->name, clm->position(), "protocol definition", classType);
                    Procedure::checkArgumentCount(clm->arguments.size(), method->arguments.size(), method->name,
                                                  clm->position(), "protocol definition", classType);
                    for (int i = 0; i < method->arguments.size(); i++) {
                        Procedure::checkArgument(clm->arguments[i].type,
                                                 method->arguments[i].type.resolveOn(protocol, false), i,
                                                 clm->position(), "protocol definition", classType);
                    }
                }
                catch (CompilerErrorException &ce) {
                    printError(ce);
                }
            }
        }
        
        biggestPlaceholder.write(biggestProtocolIndex);
        smallestPlaceholder.write(smallestProtocolIndex);
    }
}

void writePackageHeader(Package *pkg, Writer &writer) {
    uint16_t l = strlen(pkg->name()) + 1;
    writer.writeUInt16(l);
    
    writer.writeBytes(pkg->name(), l);
    
    writer.writeUInt16(pkg->version().major);
    writer.writeUInt16(pkg->version().minor);
    
    writer.writeByte(pkg->requiresBinary() ? 1 : 0);
}

void analyzeClassesAndWrite(FILE *fout) {
    Writer writer(fout);
    
    auto &theStringPool = StringPool::theStringPool();
    theStringPool.poolString(EmojicodeString());
    
    writer.writeByte(ByteCodeSpecificationVersion);
    
    // Decide which classes inherit initializers, whether they agree to protocols,
    // and assign virtual table indexes before we analyze the classes!
    for (auto eclass : Class::classes()) {
        // Decide whether this eclass is eligible for initializer inheritance
        if (eclass->instanceVariables().size() == 0 && eclass->initializerList.size() == 0) {
            eclass->inheritsContructors = true;
        }
        
        if (eclass->superclass) {
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
        
        for (auto method : eclass->methodList) {
            Method *superMethod = eclass->superclass->lookupMethod(method->name);

            method->checkOverride(superMethod);
            if (superMethod) {
                method->checkPromises(superMethod, "super method", classType);
                method->vti = superMethod->vti;
            }
            else {
                method->vti = eclass->nextMethodVti++;
            }
        }
        for (auto clMethod : eclass->classMethodList) {
            ClassMethod *superMethod = eclass->superclass->lookupClassMethod(clMethod->name);
            
            clMethod->checkOverride(superMethod);
            if (superMethod) {
                clMethod->checkPromises(superMethod, "super classmethod", classType);
                clMethod->vti = superMethod->vti;
            }
            else {
                clMethod->vti = eclass->nextClassMethodVti++;
            }
        }
        
        auto subRequiredInitializerNextVti = eclass->superclass ? eclass->superclass->requiredInitializers().size() : 0;
        eclass->nextInitializerVti += eclass->requiredInitializers().size();
        for (auto initializer : eclass->initializerList) {
            Initializer *superInit = eclass->superclass->lookupInitializer(initializer->name);
            
            initializer->checkOverride(superInit);
            
            if (initializer->required) {
                if (superInit) {
                    initializer->checkPromises(superInit, "super initializer", classType);
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
    
    writer.writeUInt16(Class::classes().size());
    
    auto pkgCount = Package::packagesInOrder().size();
    
    if (pkgCount == 2) {
        writer.writeByte(1);
        
        auto pkgs = Package::packagesInOrder();
        
        writePackageHeader(pkgs.front(), writer);
        
        auto classWritten = false;
        
        for (auto cl : pkgs.front()->classes()) {
            if (classWritten) {
                writer.writeByte(1);
            }
            
            analyzeClass(Type(cl), writer);
            classWritten = true;
        }
        for (auto cl : (*std::next(pkgs.begin(), 1))->classes()) {
            if (classWritten) {
                writer.writeByte(1);
            }
            
            analyzeClass(Type(cl), writer);
            classWritten = true;
        }
        
        writer.writeByte(0);
    }
    else {
        if (pkgCount > 256) {
            throw CompilerErrorException(Package::packagesInOrder().back()->position(),
                                         "You exceeded the maximum of 256 packages.");
        }
        
        writer.writeByte(pkgCount);
        
        for (auto pkg : Package::packagesInOrder()) {
            auto classWritten = false;
            
            writePackageHeader(pkg, writer);
            
            for (auto cl : pkg->classes()) {
                if (classWritten) {
                    writer.writeByte(1);
                }
                
                analyzeClass(Type(cl), writer);
                classWritten = true;
            }
            
            writer.writeByte(0);
        }
    }
    
    writer.writeUInt16(theStringPool.strings().size());
    for (auto string : theStringPool.strings()) {
        writer.writeUInt16(string.size());
        
        for (auto c : string) {
            writer.writeEmojicodeChar(c);
        }
    }
    
    writer.writeUInt16(startingFlag.eclass->index);
    writer.writeUInt16(startingFlag.eclass->lookupClassMethod(E_CHEQUERED_FLAG)->vti);
}