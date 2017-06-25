//
//  CodeGenerator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 19/09/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "CodeGenerator.hpp"
#include "CallableScoper.hpp"
#include "Class.hpp"
#include "DiscardingFunctionWriter.hpp"
#include "EmojicodeCompiler.hpp"
#include "FunctionPAG.hpp"
#include "Protocol.hpp"
#include "StringPool.hpp"
#include "TypeDefinitionFunctional.hpp"
#include "ValueType.hpp"
#include <cstring>
#include <vector>

template <typename T>
int writeUsed(const std::vector<T *> &functions, Writer &writer) {
    int counter = 0;
    for (auto function : functions) {
        if (function->used()) {
            writer.writeFunction(function);
            counter++;
        }
    }
    return counter;
}

template <typename T>
void compileUnused(const std::vector<T *> &functions) {
    for (auto function : functions) {
        if (!function->used() && !function->isNative()) {
            DiscardingFunctionWriter writer = DiscardingFunctionWriter();
            generateCodeForFunction(function, writer);
        }
    }
}
void generateCodeForFunction(Function *function, FunctionWriter &w) {
    CallableScoper scoper = CallableScoper();
    if (FunctionPAG::hasInstanceScope(function->compilationMode())) {
        scoper = CallableScoper(&function->owningType().typeDefinitionFunctional()->instanceScope());
    }
    FunctionPAG(*function, function->owningType().disableSelfResolving(), w, scoper).compile();
}

void writeProtocolTable(TypeDefinitionFunctional *typeDefinitionFunctional, Writer &writer) {
    writer.writeUInt16(typeDefinitionFunctional->protocols().size());
    if (!typeDefinitionFunctional->protocols().empty()) {
        auto biggestPlaceholder = writer.writePlaceholder<uint16_t>();
        auto smallestPlaceholder = writer.writePlaceholder<uint16_t>();

        uint_fast16_t smallestProtocolIndex = UINT_FAST16_MAX;
        uint_fast16_t biggestProtocolIndex = 0;

        for (const Type &protocol : typeDefinitionFunctional->protocols()) {
            writer.writeUInt16(protocol.protocol()->index);

            if (protocol.protocol()->index > biggestProtocolIndex) {
                biggestProtocolIndex = protocol.protocol()->index;
            }
            if (protocol.protocol()->index < smallestProtocolIndex) {
                smallestProtocolIndex = protocol.protocol()->index;
            }

            writer.writeUInt16(protocol.protocol()->methodList().size());

            for (auto method : protocol.protocol()->methodList()) {
                auto layerName = method->protocolBoxingLayerName(protocol.protocol()->name());
                Function *clm = typeDefinitionFunctional->lookupMethod(layerName);
                if (clm == nullptr) {
                    clm = typeDefinitionFunctional->lookupMethod(method->name());
                }
                writer.writeUInt16(clm->vtiForUse());
            }
        }

        biggestPlaceholder.write(biggestProtocolIndex);
        smallestPlaceholder.write(smallestProtocolIndex);
    }
}

void writeClass(const Type &classType, Writer &writer) {
    auto eclass = classType.eclass();

    writer.writeEmojicodeChar(eclass->name()[0]);
    if (eclass->superclass() != nullptr) {
        writer.writeUInt16(eclass->superclass()->index);
    }
    else {
        // If the class does not have a superclass the own index is written.
        writer.writeUInt16(eclass->index);
    }

    writer.writeUInt16(eclass->size());
    writer.writeUInt16(eclass->fullMethodCount());
    writer.writeByte(eclass->inheritsInitializers() ? 1 : 0);
    writer.writeUInt16(eclass->fullInitializerCount());

    if (eclass->fullInitializerCount() > 65535) {
        throw CompilerError(eclass->position(), "More than 65535 initializers in class.");
    }
    if (eclass->fullMethodCount() > 65535) {
        throw CompilerError(eclass->position(), "More than 65535 methods in class.");
    }

    writer.writeUInt16(eclass->usedMethodCount());
    writer.writeUInt16(eclass->usedInitializerCount());

    writeUsed(eclass->methodList(), writer);
    writeUsed(eclass->typeMethodList(), writer);
    writeUsed(eclass->initializerList(), writer);

    writeProtocolTable(eclass, writer);

    std::vector<ObjectVariableInformation> information;
    for (auto variable : eclass->instanceScope().map()) {
        variable.second.type().objectVariableRecords(variable.second.id(), information);
    }

    writer.writeUInt16(information.size());

    for (auto info : information) {
        writer.writeUInt16(info.index);
        writer.writeUInt16(info.conditionIndex);
        writer.writeUInt16(static_cast<uint16_t>(info.type));
    }
}

void writePackageHeader(Package *pkg, Writer &writer) {
    if (pkg->requiresBinary()) {
        size_t l = pkg->name().size() + 1;
        writer.writeByte(l);
        writer.writeBytes(pkg->name().c_str(), l);

        writer.writeUInt16(pkg->version().major);
        writer.writeUInt16(pkg->version().minor);
    }
    else {
        writer.writeByte(0);
    }

    writer.writeUInt16(pkg->classes().size());
}

void generateCode(Writer &writer) {
    auto &theStringPool = StringPool::theStringPool();
    theStringPool.poolString(EmojicodeString());

    Function::start->setVtiProvider(&Function::pureFunctionsProvider);
    Function::start->vtiForUse();

    for (auto vt : ValueType::valueTypes()) {  // Must be processed first, different sizes
        vt->finalize();
    }
    for (auto eclass : Class::classes()) {  // Can be processed afterwards, all pointers are 1 word
        eclass->finalize();
    }

    while (!Function::compilationQueue.empty()) {
        Function *function = Function::compilationQueue.front();
        generateCodeForFunction(function, function->writer_);
        Function::compilationQueue.pop();
    }

    if (BoxIDProvider::maxBoxIndetifier() > 2147483647) {
        throw CompilerError(SourcePosition(0, 0, ""), "More than 2147483647 box identifiers in use.");
    }

    writer.writeUInt16(Class::classes().size());
    writer.writeUInt16(Function::functionCount());

    auto pkgCount = Package::packagesInOrder().size();

    if (pkgCount > 255) {
        throw CompilerError(Package::packagesInOrder().back()->position(), "You exceeded the maximum of 255 packages.");
    }

    writer.writeByte(pkgCount);

    for (auto pkg : Package::packagesInOrder()) {
        writePackageHeader(pkg, writer);

        for (auto cl : pkg->classes()) {
            writeClass(Type(cl, false), writer);
        }

        auto placeholder = writer.writePlaceholder<uint16_t>();
        placeholder.write(writeUsed(pkg->functions(), writer));
    }

    uint32_t smallestBoxIdentifier = UINT16_MAX;
    uint32_t biggestBoxIdentifier = 0;
    int vtWithProtocolsCount = 0;
    auto tableSizePlaceholder = writer.writePlaceholder<uint16_t>();
    auto smallestPlaceholder = writer.writePlaceholder<uint16_t>();
    auto countPlaceholder = writer.writePlaceholder<uint16_t>();
    for (auto vt : ValueType::valueTypes()) {
        if (!vt->protocols().empty()) {
            for (auto idPair : vt->genericIds()) {
                writer.writeUInt16(idPair.second);
                writeProtocolTable(vt, writer);
                if (idPair.second < smallestBoxIdentifier) {
                    smallestBoxIdentifier = idPair.second;
                }
                if (idPair.second > biggestBoxIdentifier) {
                    biggestBoxIdentifier = idPair.second;
                }
                vtWithProtocolsCount++;
            }
        }
    }
    countPlaceholder.write(vtWithProtocolsCount);
    tableSizePlaceholder.write(vtWithProtocolsCount > 0 ? biggestBoxIdentifier - smallestBoxIdentifier + 1 : 0);
    smallestPlaceholder.write(smallestBoxIdentifier);

    writer.writeUInt16(theStringPool.strings().size());
    for (auto string : theStringPool.strings()) {
        writer.writeUInt16(string.size());

        for (auto c : string) {
            writer.writeEmojicodeChar(c);
        }
    }

    for (auto eclass : Class::classes()) {
        compileUnused(eclass->methodList());
        compileUnused(eclass->initializerList());
        compileUnused(eclass->typeMethodList());
    }

    for (auto vt : ValueType::valueTypes()) {
        compileUnused(vt->methodList());
        compileUnused(vt->initializerList());
        compileUnused(vt->typeMethodList());
    }
}
