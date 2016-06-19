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
#include "ValueType.hpp"

void analyzeClass(Type classType, Writer &writer) {
    auto eclass = classType.eclass();
    
    writer.writeEmojicodeChar(eclass->name());
    if (eclass->superclass) {
        writer.writeUInt16(eclass->superclass->index);
    }
    else {
        // If the class does not have a superclass the own index is written.
        writer.writeUInt16(eclass->index);
    }
    
    Scope objectScope;
    
    // Get the ID offset for this class by summing up all superclasses instance variable counts
    int offset = 0;
    for (Class *aClass = eclass->superclass; aClass != nullptr; aClass = aClass->superclass) {
        offset += aClass->instanceVariables().size();
    }
    auto instanceVariableCount = eclass->instanceVariables().size() + offset;
    if (instanceVariableCount > 65536) {
        throw CompilerErrorException(eclass->position(), "You exceeded the limit of 65,536 instance variables.");
    }
    writer.writeUInt16(instanceVariableCount);
    
    // Number of methods inclusive superclass
    writer.writeUInt16(eclass->nextMethodVti);
    // Initializer inclusive superclass
    writer.writeByte(eclass->inheritsContructors ? 1 : 0);
    writer.writeUInt16(eclass->nextInitializerVti);
    
    writer.writeUInt16(eclass->methodList().size() + eclass->classMethodList().size());
    writer.writeUInt16(eclass->initializerList().size());
    
    for (Variable var : eclass->instanceVariables()) {
        var.setId(offset++);
        objectScope.setLocalVariable(var.definitionToken.value, var);
    }
    
    for (auto method : eclass->methodList()) {
        auto scoper = CallableScoper(&objectScope);
        StaticFunctionAnalyzer::writeAndAnalyzeFunction(method, writer, classType.disableSelfResolving(), scoper,
                                                        StaticFunctionAnalyzerMode::ObjectMethod, false);
    }
    
    for (auto classMethod : eclass->classMethodList()) {
        auto scoper = CallableScoper();
        StaticFunctionAnalyzer::writeAndAnalyzeFunction(classMethod, writer, classType.disableSelfResolving(), scoper,
                                                        StaticFunctionAnalyzerMode::ClassMethod, true);
    }
    
    for (auto initializer : eclass->initializerList()) {
        auto scoper = CallableScoper(&objectScope);
        StaticFunctionAnalyzer::writeAndAnalyzeFunction(initializer, writer, classType.disableSelfResolving(), scoper,
                                                        StaticFunctionAnalyzerMode::ObjectInitializer, false);
    }
    
    if (eclass->instanceVariables().size() > 0 && eclass->initializerList().size() == 0) {
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
            writer.writeUInt16(protocol.protocol()->index);
            
            if (protocol.protocol()->index > biggestProtocolIndex) {
                biggestProtocolIndex = protocol.protocol()->index;
            }
            if (protocol.protocol()->index < smallestProtocolIndex) {
                smallestProtocolIndex = protocol.protocol()->index;
            }
            
            writer.writeUInt16(protocol.protocol()->methods().size());
            
            for (auto method : protocol.protocol()->methods()) {
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
                    
                    writer.writeUInt16(clm->vti());
                    Function::checkReturnPromise(clm->returnType, method->returnType.resolveOn(protocol, false),
                                                  method->name, clm->position(), "protocol definition", classType);
                    Function::checkArgumentCount(clm->arguments.size(), method->arguments.size(), method->name,
                                                  clm->position(), "protocol definition", classType);
                    for (int i = 0; i < method->arguments.size(); i++) {
                        Function::checkArgument(clm->arguments[i].type,
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

void analyzeValueType(ValueType *vt, Writer &writer) {
    writer.writeEmojicodeChar(vt->name());
    writer.writeUInt16(vt->methodList().size() + vt->initializerList().size() + vt->classMethodList().size());
    for (auto f : vt->methodList()) {
        auto scoper = CallableScoper();
        StaticFunctionAnalyzer::writeAndAnalyzeFunction(f, writer, f->owningType, scoper,
                                                        StaticFunctionAnalyzerMode::ThisContextFunction, false);
    }
    for (auto f : vt->initializerList()) {
        auto scoper = CallableScoper();
        StaticFunctionAnalyzer::writeAndAnalyzeFunction(f, writer, f->owningType, scoper,
                                                        StaticFunctionAnalyzerMode::ThisContextFunction, false);
    }
    for (auto f : vt->classMethodList()) {
        auto scoper = CallableScoper();
        StaticFunctionAnalyzer::writeAndAnalyzeFunction(f, writer, f->owningType, scoper,
                                                        StaticFunctionAnalyzerMode::Function, true);
    }
}

void writePackageHeader(Package *pkg, Writer &writer, uint16_t classCount) {
    if (pkg->requiresBinary()) {
        uint16_t l = strlen(pkg->name()) + 1;
        writer.writeByte(l);
        
        writer.writeBytes(pkg->name(), l);
        
        writer.writeUInt16(pkg->version().major);
        writer.writeUInt16(pkg->version().minor);
    }
    else {
        writer.writeByte(0);
    }
    
    writer.writeUInt16(classCount);
}

void writePackageHeader(Package *pkg, Writer &writer) {
    writePackageHeader(pkg, writer, pkg->classes().size());
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
        if (eclass->instanceVariables().size() == 0 && eclass->initializerList().size() == 0) {
            eclass->inheritsContructors = true;
        }
        
        if (eclass->superclass) {
            eclass->nextInitializerVti = eclass->inheritsContructors ? eclass->superclass->nextInitializerVti : 0;
            eclass->nextMethodVti = eclass->superclass->nextMethodVti;
        }
        else {
            eclass->nextInitializerVti = 0;
            eclass->nextMethodVti = 0;
        }
        
        Type classType = Type(eclass);
        
        for (auto method : eclass->methodList()) {
            auto superMethod = eclass->superclass ? eclass->superclass->lookupMethod(method->name) : NULL;

            method->checkOverride(superMethod);
            if (superMethod) {
                method->checkPromises(superMethod, "super method", classType);
                method->setVti(superMethod->vti());
            }
            else {
                method->setVti(eclass->nextMethodVti++);
            }
        }
        for (auto clMethod : eclass->classMethodList()) {
            auto superMethod = eclass->superclass ? eclass->superclass->lookupClassMethod(clMethod->name) : NULL;
            
            clMethod->checkOverride(superMethod);
            if (superMethod) {
                clMethod->checkPromises(superMethod, "super classmethod", classType);
                clMethod->setVti(superMethod->vti());
            }
            else {
                clMethod->setVti(eclass->nextMethodVti++);
            }
        }
        
        auto subRequiredInitializerNextVti = eclass->superclass ? eclass->superclass->requiredInitializers().size() : 0;
        eclass->nextInitializerVti += eclass->requiredInitializers().size();
        for (auto initializer : eclass->initializerList()) {
            auto superInit = eclass->superclass ? eclass->superclass->lookupInitializer(initializer->name) : NULL;
            
            initializer->checkOverride(superInit);
            
            if (initializer->required) {
                if (superInit) {
                    initializer->checkPromises(superInit, "super initializer", classType);
                    initializer->setVti(superInit->vti());
                }
                else {
                    initializer->setVti(static_cast<int>(subRequiredInitializerNextVti++));
                }
            }
            else {
                initializer->setVti(eclass->nextInitializerVti++);
            }
        }
    }
    
    int functionVti = 0;
    Function::start->setVti(functionVti++);

    for (auto vt : ValueType::valueTypes()) {
        for (auto f : vt->methodList()) {
            f->setVti(functionVti++);
        }
        for (auto f : vt->classMethodList()) {
            f->setVti(functionVti++);
        }
        for (auto f : vt->initializerList()) {
            f->setVti(functionVti++);
        }
    }
    
    writer.writeUInt16(Class::classes().size());
    
    auto pkgCount = Package::packagesInOrder().size();
    
    if (pkgCount == 2) {
        writer.writeByte(1);
        
        auto pkgs = Package::packagesInOrder();
        auto underscorePackage = (*std::next(pkgs.begin(), 1));
        
        writePackageHeader(pkgs.front(), writer, pkgs.front()->classes().size() + underscorePackage->classes().size());
        
        for (auto cl : pkgs.front()->classes()) {
            analyzeClass(Type(cl), writer);
        }
        
        for (auto cl : underscorePackage->classes()) {
            analyzeClass(Type(cl), writer);
        }
    }
    else {
        if (pkgCount > 256) {
            throw CompilerErrorException(Package::packagesInOrder().back()->position(),
                                         "You exceeded the maximum of 256 packages.");
        }
        
        writer.writeByte(pkgCount);
        
        for (auto pkg : Package::packagesInOrder()) {
            writePackageHeader(pkg, writer);
            
            for (auto cl : pkg->classes()) {
                analyzeClass(Type(cl), writer);
            }
        }
    }
    
    writer.writeUInt16(functionVti);
    writer.writeUInt16(ValueType::valueTypes().size() + 1);
    writer.writeEmojicodeChar(0);
    writer.writeUInt16(1);
    auto scoper = CallableScoper();
    StaticFunctionAnalyzer::writeAndAnalyzeFunction(Function::start, writer, typeNothingness, scoper,
                                                    StaticFunctionAnalyzerMode::Function, false);

    for (auto vt : ValueType::valueTypes()) {
        analyzeValueType(vt, writer);
    }
    
    writer.writeUInt16(theStringPool.strings().size());
    for (auto string : theStringPool.strings()) {
        writer.writeUInt16(string.size());
        
        for (auto c : string) {
            writer.writeEmojicodeChar(c);
        }
    }
}