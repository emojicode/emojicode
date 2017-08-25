//
//  CodeGenerator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 19/09/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "CodeGenerator.hpp"
#include "../Application.hpp"
#include "../CompilerError.hpp"
#include "../EmojicodeCompiler.hpp"
#include "../Functions/Initializer.hpp"
#include "../Types/Class.hpp"
#include "../Types/Protocol.hpp"
#include "../Types/TypeDefinition.hpp"
#include "../Types/ValueType.hpp"
#include "FnCodeGenerator.hpp"
#include "StringPool.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

namespace EmojicodeCompiler {

template <typename T>
int writeUsed(const std::vector<T *> &functions, Writer *writer) {
    int counter = 0;
    for (auto function : functions) {
        if (function->used()) {
            writer->writeFunction(function);
            counter++;
        }
    }
    return counter;
}

void generateCodeForFunction(Function *function) {
    FnCodeGenerator(function).generate();
}

void writeProtocolTable(TypeDefinition *typeDefinition, Writer *writer) {
    std::vector<Type> usedProtocols;
    std::copy_if(typeDefinition->protocols().begin(), typeDefinition->protocols().end(),
                 std::back_inserter(usedProtocols), [](const Type &protocol) {
        auto methodList = protocol.protocol()->methodList();
        return std::any_of(methodList.begin(), methodList.end(), [](Function *f){ return f->used(); });
    });

    writer->writeUInt16(usedProtocols.size());
    if (!usedProtocols.empty()) {
        auto biggestPlaceholder = writer->writePlaceholder<uint16_t>();
        auto smallestPlaceholder = writer->writePlaceholder<uint16_t>();

        uint_fast16_t smallestProtocolIndex = UINT_FAST16_MAX;
        uint_fast16_t biggestProtocolIndex = 0;

        for (const Type &protocol : usedProtocols) {
            writer->writeUInt16(protocol.protocol()->index);

            if (protocol.protocol()->index > biggestProtocolIndex) {
                biggestProtocolIndex = protocol.protocol()->index;
            }
            if (protocol.protocol()->index < smallestProtocolIndex) {
                smallestProtocolIndex = protocol.protocol()->index;
            }

            writer->writeUInt16(protocol.protocol()->methodList().size());

            for (auto method : protocol.protocol()->methodList()) {
                auto layerName = method->protocolBoxingLayerName(protocol.protocol()->name());
                Function *clm = typeDefinition->lookupMethod(layerName);
                if (clm == nullptr) {
                    clm = typeDefinition->lookupMethod(method->name());
                }
                writer->writeUInt16(clm->used() ? clm->getVti() : 0);
            }
        }

        biggestPlaceholder.write(biggestProtocolIndex);
        smallestPlaceholder.write(smallestProtocolIndex);
    }
}

void writeClass(const Type &classType, Writer *writer) {
    auto eclass = classType.eclass();

    writer->writeEmojicodeChar(eclass->name()[0]);
    if (eclass->superclass() != nullptr) {
        writer->writeUInt16(eclass->superclass()->index);
    }
    else {
        // If the class does not have a superclass the own index is written.
        writer->writeUInt16(eclass->index);
    }

    writer->writeUInt16(eclass->size());
    writer->writeUInt16(eclass->fullMethodCount());
    writer->writeByte(eclass->inheritsInitializers() ? 1 : 0);
    writer->writeUInt16(eclass->fullInitializerCount());

    if (eclass->fullInitializerCount() > 65535) {
        throw CompilerError(eclass->position(), "More than 65535 initializers in class.");
    }
    if (eclass->fullMethodCount() > 65535) {
        throw CompilerError(eclass->position(), "More than 65535 methods in class.");
    }

    writer->writeUInt16(eclass->usedMethodCount());
    writer->writeUInt16(eclass->usedInitializerCount());

    writeUsed(eclass->methodList(), writer);
    writeUsed(eclass->typeMethodList(), writer);
    writeUsed(eclass->initializerList(), writer);

    writeProtocolTable(eclass, writer);

    std::vector<ObjectVariableInformation> information;
    for (auto variable : eclass->instanceScope().map()) {
        // TODO:    variable.second.type().objectVariableRecords(variable.second.id(), &information);
    }

    writer->writeUInt16(information.size());

    for (auto info : information) {
        writer->writeUInt16(info.index);
        writer->writeUInt16(info.conditionIndex);
        writer->writeUInt16(static_cast<uint16_t>(info.type));
    }
}

void writePackageHeader(Package *pkg, Writer *writer) {
    if (pkg->requiresBinary()) {
        size_t l = pkg->name().size() + 1;
        writer->writeByte(l);
        writer->writeBytes(pkg->name().c_str(), l);

        writer->writeUInt16(pkg->version().major);
        writer->writeUInt16(pkg->version().minor);
    }
    else {
        writer->writeByte(0);
    }

    writer->writeUInt16(pkg->classes().size());
}

void generateCode(Writer *writer, Application *app) {
    app->stringPool().pool(std::u32string());

    app->startFlagFunction()->setVtiProvider(&STIProvider::globalStiProvider);
    app->startFlagFunction()->vtiForUse();

    for (auto package : app->packagesInOrder()) {
        for (auto valueType : package->valueTypes()) {
            valueType->prepareForCG();
        }
        for (auto klass : package->classes()) {
            klass->prepareForCG();
        }
    }

    while (!app->compilationQueue.empty()) {
        Function *function = app->compilationQueue.front();
        generateCodeForFunction(function);
        app->compilationQueue.pop();
    }

    if (app->maxBoxIndetifier() > 2147483647) {
        throw CompilerError(SourcePosition(0, 0, ""), "More than 2147483647 box identifiers in use.");
    }
    if (app->protocolCount() == UINT16_MAX) {
        throw CompilerError(SourcePosition(0, 0, ""), "You exceeded the limit of 65,536 protocols.");
    }

    writer->writeUInt16(app->classCount());
    writer->writeUInt16(STIProvider::functionCount());

    if (app->packagesInOrder().size() > 255) {
        throw CompilerError(app->packagesInOrder().back()->position(), "You exceeded the maximum of 255 packages.");
    }
    writer->writeByte(app->packagesInOrder().size());

    for (auto pkg : app->packagesInOrder()) {
        writePackageHeader(pkg, writer);

        for (auto cl : pkg->classes()) {
            writeClass(Type(cl, false), writer);
        }

        auto placeholder = writer->writePlaceholder<uint16_t>();
        placeholder.write(writeUsed(pkg->functions(), writer));
    }

    uint32_t smallestBoxIdentifier = UINT16_MAX;
    uint32_t biggestBoxIdentifier = 0;
    int vtWithProtocolsCount = 0;
    auto tableSizePlaceholder = writer->writePlaceholder<uint16_t>();
    auto smallestPlaceholder = writer->writePlaceholder<uint16_t>();
    auto countPlaceholder = writer->writePlaceholder<uint16_t>();
    for (auto package : app->packagesInOrder()) {
        for (auto vt : package->valueTypes()) {
            if (!vt->protocols().empty()) {
                for (auto idPair : vt->genericIds()) {
                    writer->writeUInt16(idPair.second);
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
    }
    countPlaceholder.write(vtWithProtocolsCount);
    tableSizePlaceholder.write(vtWithProtocolsCount > 0 ? biggestBoxIdentifier - smallestBoxIdentifier + 1 : 0);
    smallestPlaceholder.write(smallestBoxIdentifier);

    auto binfo = app->boxObjectVariableInformation();
    writer->writeInstruction(binfo.size());
    for (auto information : binfo) {
        writer->writeUInt16(information.size());

        for (auto info : information) {
            writer->writeUInt16(info.index);
            writer->writeUInt16(info.conditionIndex);
            writer->writeUInt16(static_cast<uint16_t>(info.type));
        }
    }

    writer->writeUInt16(app->stringPool().strings().size());
    for (auto string : app->stringPool().strings()) {
        writer->writeUInt16(string.size());

        for (auto c : string) {
            writer->writeEmojicodeChar(c);
        }
    }
}

}  // namespace EmojicodeCompiler
