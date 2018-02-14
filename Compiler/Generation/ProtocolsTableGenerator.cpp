//
// Created by Theo Weidmann on 07.02.18.
//

#include "ProtocolsTableGenerator.hpp"
#include "Types/Type.hpp"
#include "Types/TypeDefinition.hpp"
#include "Types/Protocol.hpp"
#include "Generation/Mangler.hpp"
#include "Functions/Function.hpp"
#include "LLVMTypeHelper.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>

namespace EmojicodeCompiler {

void ProtocolsTableGenerator::createProtocolsTable(TypeDefinition *typeDef) {
    llvm::Constant* init;
    if (!typeDef->protocols().empty()) {
        auto tables = createVirtualTables(typeDef);
        auto arrayType = llvm::ArrayType::get(llvm::Type::getInt8PtrTy(context_)->getPointerTo(), tables.tables.size());
        auto protocols = new llvm::GlobalVariable(module_, arrayType, true,
                                                  llvm::GlobalValue::LinkageTypes::InternalLinkage,
                                                  llvm::ConstantArray::get(arrayType, tables.tables));

        init = llvm::ConstantStruct::get(typeHelper_.protocolsTable(), std::vector<llvm::Constant *> {
            protocols,
            llvm::ConstantInt::get(llvm::Type::getInt16Ty(context_), tables.min),
            llvm::ConstantInt::get(llvm::Type::getInt16Ty(context_), tables.max)
        });
    }
    else {
        init = llvm::ConstantAggregateZero::get(typeHelper_.protocolsTable());
    }
    typeDef->setProtocolsTable(init);
}

ProtocolsTableGenerator::ProtocolVirtualTables ProtocolsTableGenerator::createVirtualTables(TypeDefinition *typeDef) {
    std::vector<std::pair<llvm::Constant *, size_t>> protocolVirtualTablesUnordered;

    size_t min = std::numeric_limits<size_t>::max();
    size_t max = 0;

    for (auto &protocol : typeDef->protocols()) {
        auto index = protocol.protocol()->index();
        if (index < min) {
            min = index;
        }
        if (index > max) {
            max = index;
        }
        protocolVirtualTablesUnordered.emplace_back(createVirtualTable(typeDef, protocol.protocol()), index);
    }

    auto type = llvm::UndefValue::get(llvm::Type::getInt8PtrTy(context_)->getPointerTo());
    std::vector<llvm::Constant *> protocolVirtualTables(max - min + 1, type);
    for (auto &pair : protocolVirtualTablesUnordered) {
        protocolVirtualTables[pair.second - min] = pair.first;
    }

    return ProtocolVirtualTables(protocolVirtualTables, max, min);
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
