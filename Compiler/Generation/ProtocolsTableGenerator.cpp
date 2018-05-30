//
// Created by Theo Weidmann on 07.02.18.
//

#include "ProtocolsTableGenerator.hpp"
#include "Functions/Function.hpp"
#include "Generation/Mangler.hpp"
#include "LLVMTypeHelper.hpp"
#include "Types/Protocol.hpp"
#include "Types/Type.hpp"
#include "Types/TypeDefinition.hpp"
#include "Types/Class.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>

namespace EmojicodeCompiler {

void ProtocolsTableGenerator::createProtocolsTable(const Type &type) {
    std::map<Type, llvm::Constant *> tables;

    for (auto &protocol : type.typeDefinition()->protocols()) {
        tables.emplace(protocol.unboxed(), createVirtualTable(type, protocol));
    }

    type.typeDefinition()->setProtocolTables(std::move(tables));
}

void ProtocolsTableGenerator::declareImportedProtocolsTable(const Type &type) {
    std::map<Type, llvm::Constant *> tables;

    for (auto &protocol : type.typeDefinition()->protocols()) {
        tables.emplace(protocol.unboxed(), getConformanceVariable(type, protocol, nullptr));
    }

    type.typeDefinition()->setProtocolTables(std::move(tables));
}

llvm::GlobalVariable* ProtocolsTableGenerator::multiprotocol(const Type &multiprotocol, const Type &conformer) {
    auto pair = std::make_pair(multiprotocol.unboxed(), conformer.typeDefinition());
    auto it = multiprotocolTables_.find(pair);
    if (it != multiprotocolTables_.end()) {
        return it->second;
    }

    std::vector<llvm::Constant *> virtualTable;
    for (auto protocol : multiprotocol.protocols()) {
        virtualTable.emplace_back(conformer.typeDefinition()->protocolTableFor(protocol.unboxed()));
    }

    auto arrayType = llvm::ArrayType::get(typeHelper_.protocolConformance()->getPointerTo(),
                                          multiprotocol.protocols().size());
    auto array = llvm::ConstantArray::get(arrayType, virtualTable);
    auto var = new llvm::GlobalVariable(module_, arrayType, true, llvm::GlobalValue::LinkageTypes::PrivateLinkage,
                                        array, mangleMultiprotocolConformance(multiprotocol, conformer));
    multiprotocolTables_.emplace(pair, var);
    return var;
}

llvm::GlobalVariable* ProtocolsTableGenerator::createVirtualTable(const Type &type, const Type &protocol) {
    auto typeDef = type.typeDefinition();
    auto arrayType = llvm::ArrayType::get(llvm::Type::getInt8PtrTy(context_), protocol.protocol()->methodList().size());

    std::vector<llvm::Constant *> virtualTable;
    virtualTable.resize(protocol.protocol()->methodList().size());

    for (auto protocolMethod : protocol.protocol()->methodList()) {
        for (auto reification : protocolMethod->reificationMap()) {
            auto implFunction = typeDef->lookupMethod(protocolMethod->protocolBoxingThunk(protocol.protocol()->name()),
                                                      protocolMethod->isImperative());
            if (implFunction == nullptr) {
                implFunction = typeDef->lookupMethod(protocolMethod->name(), protocolMethod->isImperative());
            }
            auto implReif = implFunction->reificationFor(reification.first).function;
            assert(implReif != nullptr);
            virtualTable[reification.second.entity.vti()] = implReif;
        }
    }

    auto array = llvm::ConstantArray::get(arrayType, virtualTable);
    auto arrayVar = new llvm::GlobalVariable(module_, arrayType, true, llvm::GlobalValue::LinkageTypes::PrivateLinkage,
                                             array);
    auto load = llvm::ConstantInt::get(llvm::Type::getInt1Ty(context_),
                                       (type.type() == TypeType::Class || typeHelper_.isRemote(type)) ? 1 : 0);
    auto conformance = llvm::ConstantStruct::get(typeHelper_.protocolConformance(), {load, arrayVar});
    return getConformanceVariable(type, protocol, conformance);
}

llvm::GlobalVariable* ProtocolsTableGenerator::getConformanceVariable(const Type &type, const Type &protocol,
                                                                      llvm::Constant *conformance) const {
    return new llvm::GlobalVariable(module_, typeHelper_.protocolConformance(), true,
                                    llvm::GlobalValue::ExternalLinkage, conformance,
                                    mangleProtocolConformance(type, protocol));
}

}  // namespace EmojicodeCompiler
