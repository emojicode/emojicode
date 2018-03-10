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
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>

namespace EmojicodeCompiler {

void ProtocolsTableGenerator::createProtocolsTable(TypeDefinition *typeDef) {
    std::map<Type, llvm::Constant*> tables;

    for (auto &protocol : typeDef->protocols()) {
        tables.emplace(protocol, createVirtualTable(typeDef, protocol.protocol()));
    }

    typeDef->setProtocolTables(std::move(tables));
}

llvm::GlobalVariable* ProtocolsTableGenerator::createVirtualTable(TypeDefinition *typeDef, Protocol *protocol) {
    auto type = llvm::ArrayType::get(llvm::Type::getInt8PtrTy(context_), protocol->methodList().size());

    std::vector<llvm::Constant *> virtualTable;
    virtualTable.resize(protocol->methodList().size());

    for (auto protocolMethod : protocol->methodList()) {
        for (auto reification : protocolMethod->reificationMap()) {
            auto implFunction = typeDef->lookupMethod(protocolMethod->name(), protocolMethod->isImperative());
            auto implReif = implFunction->reificationFor(reification.first).function;
            assert(implReif != nullptr);
            virtualTable[reification.second.entity.vti()] = implReif;
        }
    }

    auto array = llvm::ConstantArray::get(type, virtualTable);
    return new llvm::GlobalVariable(module_, type, true, llvm::GlobalValue::LinkageTypes::InternalLinkage, array);
}

}  // namespace EmojicodeCompiler
