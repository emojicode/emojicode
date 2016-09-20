//
//  CodeGenerator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 19/09/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include <cstring>
#include "CodeGenerator.hpp"
#include "CallableParserAndGenerator.hpp"
#include "Protocol.hpp"
#include "CallableScoper.hpp"
#include "Class.hpp"
#include "utf8.h"
#include "EmojicodeCompiler.hpp"
#include "StringPool.hpp"
#include "ValueType.hpp"

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
    
    writer.writeUInt16(eclass->methodList().size() + eclass->classMethodList().size());
    writer.writeUInt16(eclass->initializerList().size());
    
    for (auto method : eclass->methodList()) {
        auto scoper = CallableScoper(&eclass->objectScope());
        CallableParserAndGenerator::writeAndAnalyzeFunction(method, writer, classType.disableSelfResolving(), scoper,
                                                        CallableParserAndGeneratorMode::ObjectMethod, false);
    }
    
    for (auto classMethod : eclass->classMethodList()) {
        auto scoper = CallableScoper();
        CallableParserAndGenerator::writeAndAnalyzeFunction(classMethod, writer, classType.disableSelfResolving(), scoper,
                                                        CallableParserAndGeneratorMode::ClassMethod, true);
    }
    
    for (auto initializer : eclass->initializerList()) {
        auto scoper = CallableScoper(&eclass->objectScope());
        CallableParserAndGenerator::writeAndAnalyzeFunction(initializer, writer, classType.disableSelfResolving(), scoper,
                                                        CallableParserAndGeneratorMode::ObjectInitializer, false);
    }
    
    
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

void writeValueType(ValueType *vt, Writer &writer) {
    writer.writeEmojicodeChar(vt->name());
    writer.writeUInt16(vt->methodList().size() + vt->initializerList().size() + vt->classMethodList().size());
    for (auto f : vt->methodList()) {
        auto scoper = CallableScoper();
        CallableParserAndGenerator::writeAndAnalyzeFunction(f, writer, f->owningType(), scoper,
                                                        CallableParserAndGeneratorMode::ThisContextFunction, false);
    }
    for (auto f : vt->initializerList()) {
        auto scoper = CallableScoper();
        CallableParserAndGenerator::writeAndAnalyzeFunction(f, writer, f->owningType(), scoper,
                                                        CallableParserAndGeneratorMode::ThisContextFunction, false);
    }
    for (auto f : vt->classMethodList()) {
        auto scoper = CallableScoper();
        CallableParserAndGenerator::writeAndAnalyzeFunction(f, writer, f->owningType(), scoper,
                                                        CallableParserAndGeneratorMode::Function, true);
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

void generateCode(Writer &writer) {
    auto &theStringPool = StringPool::theStringPool();
    theStringPool.poolString(EmojicodeString());
    
    for (auto eclass : Class::classes()) {
        eclass->finalize();
    }
    
    Function::start->setVti(Function::nextFunctionVti());
    
    for (auto vt : ValueType::valueTypes()) {
        vt->finalize();
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
    CallableParserAndGenerator::writeAndAnalyzeFunction(Function::start, writer, typeNothingness, scoper,
                                                    CallableParserAndGeneratorMode::Function, false);
    
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
}
