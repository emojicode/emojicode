//
//  CodeGenerator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 19/09/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include <cstring>
#include <vector>
#include "CodeGenerator.hpp"
#include "CallableParserAndGenerator.hpp"
#include "Protocol.hpp"
#include "CallableScoper.hpp"
#include "Class.hpp"
#include "utf8.h"
#include "EmojicodeCompiler.hpp"
#include "StringPool.hpp"
#include "ValueType.hpp"
#include "DiscardingCallableWriter.hpp"

template <typename T>
void writeAssigned(const std::vector<T *> &functions, Writer &writer) {
    for (auto function : functions) {
        if (function->assigned()) {
            writer.writeFunction(function);
        }
    }
}

template <typename T>
void compileUnassigned(const std::vector<T *> &functions) {
    for (auto function : functions) {
        if (!function->assigned() && !function->native) {
            DiscardingCallableWriter writer = DiscardingCallableWriter();
            generateCodeForFunction(function, writer);
        }
    }
}
void generateCodeForFunction(Function *function, CallableWriter &w) {
    CallableScoper scoper = CallableScoper();
    if (function->compilationMode() == CallableParserAndGeneratorMode::ObjectMethod ||
        function->compilationMode() == CallableParserAndGeneratorMode::ObjectInitializer) {
        scoper = CallableScoper(&function->owningType().eclass()->objectScope());
    }
    CallableParserAndGenerator::writeAndAnalyzeFunction(function, w, function->owningType().disableSelfResolving(),
                                                        scoper, function->compilationMode());
}

void writeClass(Type classType, Writer &writer) {
    auto eclass = classType.eclass();
    
    writer.writeEmojicodeChar(eclass->name());
    if (eclass->superclass()) {
        writer.writeUInt16(eclass->superclass()->index);
    }
    else {
        // If the class does not have a superclass the own index is written.
        writer.writeUInt16(eclass->index);
    }
    
    writer.writeUInt16(eclass->fullInstanceVariableCount());
    writer.writeUInt16(eclass->fullMethodCount());
    writer.writeByte(eclass->inheritsInitializers() ? 1 : 0);
    writer.writeUInt16(eclass->fullInitializerCount());
    
    writer.writeUInt16(eclass->assignedMethodCount());
    writer.writeUInt16(eclass->assignedInitializerCount());
    
    writeAssigned(eclass->methodList(), writer);
    writeAssigned(eclass->classMethodList(), writer);
    writeAssigned(eclass->initializerList(), writer);
    
    writer.writeUInt16(eclass->protocols().size());
    
    if (eclass->protocols().size() > 0) {
        auto biggestPlaceholder = writer.writePlaceholder<uint16_t>();
        auto smallestPlaceholder = writer.writePlaceholder<uint16_t>();
        
        uint_fast16_t smallestProtocolIndex = UINT_FAST16_MAX;
        uint_fast16_t biggestProtocolIndex = 0;
        
        for (Type protocol : eclass->protocols()) {
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
                    Function *clm = eclass->lookupMethod(method->name);
                    if (clm == nullptr) {
                        auto className = classType.toString(typeNothingness, true);
                        auto protocolName = protocol.toString(typeNothingness, true);
                        ecCharToCharStack(method->name, ms);
                        throw CompilerErrorException(eclass->position(),
                                                     "Class %s does not agree to protocol %s: Method %s is missing.",
                                                     className.c_str(), protocolName.c_str(), ms);
                    }
                    
                    writer.writeUInt16(clm->vtiForUse());
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

void writeValueType(ValueType *vt, Writer &writer) {
    writer.writeEmojicodeChar(vt->name());
    writer.writeUInt16(vt->assignedFunctionCount());
    writeAssigned(vt->methodList(), writer);
    writeAssigned(vt->classMethodList(), writer);
    writeAssigned(vt->initializerList(), writer);
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


void generateCode(Writer &writer) {
    auto &theStringPool = StringPool::theStringPool();
    theStringPool.poolString(EmojicodeString());
    
    Function::start->setVti(Function::nextFunctionVti());
    
    for (auto eclass : Class::classes()) {
        eclass->finalize();
    }
    
    for (auto vt : ValueType::valueTypes()) {
        vt->finalize();
    }
    
    while (!Function::compilationQueue.empty()) {
        Function *function = Function::compilationQueue.front();
        generateCodeForFunction(function, function->writer_);
        Function::compilationQueue.pop();
    }
    
    writer.writeByte(ByteCodeSpecificationVersion);
    writer.writeUInt16(Class::classes().size());
    
    auto pkgCount = Package::packagesInOrder().size();
    
    if (pkgCount == 2) {
        writer.writeByte(1);
        
        auto pkgs = Package::packagesInOrder();
        auto underscorePackage = (*std::next(pkgs.begin(), 1));
        
        writePackageHeader(pkgs.front(), writer, pkgs.front()->classes().size() + underscorePackage->classes().size());
        
        for (auto cl : pkgs.front()->classes()) {
            writeClass(Type(cl), writer);
        }
        
        for (auto cl : underscorePackage->classes()) {
            writeClass(Type(cl), writer);
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
                writeClass(Type(cl), writer);
            }
        }
    }
    
    writer.writeUInt16(Function::functionCount());
    writer.writeUInt16(ValueType::valueTypes().size() + 1);
    writer.writeEmojicodeChar(0);
    writer.writeUInt16(1);
    auto scoper = CallableScoper();
    
    writer.writeFunction(Function::start);

    for (auto vt : ValueType::valueTypes()) {
        writeValueType(vt, writer);
    }
    
    writer.writeUInt16(theStringPool.strings().size());
    for (auto string : theStringPool.strings()) {
        writer.writeUInt16(string.size());
        
        for (auto c : string) {
            writer.writeEmojicodeChar(c);
        }
    }
    
    for (auto eclass : Class::classes()) {
        compileUnassigned(eclass->methodList());
        compileUnassigned(eclass->initializerList());
        compileUnassigned(eclass->classMethodList());
    }
    
    for (auto vt : ValueType::valueTypes()) {
        compileUnassigned(vt->methodList());
        compileUnassigned(vt->initializerList());
        compileUnassigned(vt->classMethodList());
    }
}
