//
// Created by Theo Weidmann on 07.02.18.
//

#include "ProtocolsTableGenerator.hpp"
#include "CodeGenerator.hpp"
#include "Functions/Function.hpp"
#include "Generation/Declarator.hpp"
#include "Generation/Mangler.hpp"
#include "LLVMTypeHelper.hpp"
#include "Types/Class.hpp"
#include "Types/Protocol.hpp"
#include "Types/Type.hpp"
#include "Types/TypeDefinition.hpp"
#include "Types/ValueType.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>

namespace EmojicodeCompiler {

llvm::Constant* ProtocolsTableGenerator::createProtocolTable(TypeDefinition *typeDef) {
    std::vector<llvm::Constant *> entries;
    entries.reserve(typeDef->protocolTables().size());
    for (auto &entry : typeDef->protocolTables()) {
        entries.emplace_back(llvm::ConstantStruct::get(generator_->typeHelper().protocolConformanceEntry(), {
            generator_->runTimeTypeInfoForProtocol(entry.first), entry.second
        }));
    }

    entries.emplace_back(llvm::Constant::getNullValue(generator_->typeHelper().protocolConformanceEntry()));
    auto arrayType = llvm::ArrayType::get(generator_->typeHelper().protocolConformanceEntry(), entries.size());
    auto array = new llvm::GlobalVariable(*generator_->module(), arrayType, true,
                                          llvm::GlobalValue::LinkageTypes::PrivateLinkage,
                                          llvm::ConstantArray::get(arrayType, entries));
    return buildConstant00Gep(arrayType, array, generator_->context());
}

void ProtocolsTableGenerator::generate(const Type &type) {
    std::map<Type, llvm::Constant *> tables;
    auto boxInfo = generator_->boxInfoFor(type);

    for (auto &protocol : type.typeDefinition()->protocols()) {
        auto conformance = createDispatchTable(type, protocol->type(), boxInfo);
        tables.emplace(protocol->type().unboxed(), conformance);
    }

    type.typeDefinition()->setProtocolTables(std::move(tables));
}

void ProtocolsTableGenerator::declareImported(const Type &type) {
    std::map<Type, llvm::Constant *> tables;
    if (type.type() != TypeType::Class) {
        type.unboxed().valueType()->setBoxInfo(generator_->declarator().declareBoxInfo(mangleBoxInfoName(type)));
    }

    for (auto &protocol : type.typeDefinition()->protocols()) {
        tables.emplace(protocol->type().unboxed(), getConformanceVariable(type, protocol->type(), nullptr));
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
    for (auto &protocol : multiprotocol.protocols()) {
        virtualTable.emplace_back(conformer.typeDefinition()->protocolTableFor(protocol.unboxed()));
    }

    auto arrayType = llvm::ArrayType::get(generator_->typeHelper().protocolConformance()->getPointerTo(),
                                          multiprotocol.protocols().size());
    auto array = llvm::ConstantArray::get(arrayType, virtualTable);
    auto var = new llvm::GlobalVariable(*generator_->module(), arrayType, true,
                                        llvm::GlobalValue::LinkageTypes::PrivateLinkage,
                                        array, mangleMultiprotocolConformance(multiprotocol, conformer));
    multiprotocolTables_.emplace(pair, var);
    return var;
}

llvm::GlobalVariable* ProtocolsTableGenerator::createDispatchTable(const Type &type, const Type &protocol,
                                                                  llvm::Constant *boxInfo) {
    auto typeDef = type.typeDefinition();
    auto arrayType = llvm::ArrayType::get(llvm::Type::getInt8PtrTy(generator_->context()),
                                          protocol.protocol()->methodList().size());

    std::vector<llvm::Constant *> virtualTable;
    virtualTable.resize(protocol.protocol()->methodList().size());

    for (auto protocolMethod : protocol.protocol()->methodList()) {
        for (auto reification : protocolMethod->reificationMap()) {
            auto implFunction = typeDef->lookupMethod(protocolMethod->protocolBoxingThunk(protocol.protocol()->name()),
                                                      protocolMethod->mood());
            if (implFunction == nullptr) {
                implFunction = typeDef->lookupMethod(protocolMethod->name(), protocolMethod->mood());
            }
            auto implReif = implFunction->reificationFor(reification.first).function;
            assert(implReif != nullptr);
            virtualTable[reification.second.entity.vti()] = implReif;
        }
    }

    auto array = llvm::ConstantArray::get(arrayType, virtualTable);
    auto arrayVar = new llvm::GlobalVariable(*generator_->module(), arrayType, true,
                                             llvm::GlobalValue::LinkageTypes::PrivateLinkage, array);
    auto avGep = buildConstant00Gep(arrayType, arrayVar, generator_->context());

    auto load = llvm::ConstantInt::get(llvm::Type::getInt1Ty(generator_->context()),
                                       (type.type() == TypeType::Class ||
                                        generator_->typeHelper().isRemote(type)) ? 1 : 0);
    auto conformance = llvm::ConstantStruct::get(generator_->typeHelper().protocolConformance(),
                                                 {load, avGep, llvm::ConstantExpr::getBitCast(boxInfo, generator_->typeHelper().boxInfo()->getPointerTo()), typeDef->boxRetainRelease().first, typeDef->boxRetainRelease().second });
    return getConformanceVariable(type, protocol, conformance);
}

llvm::GlobalVariable* ProtocolsTableGenerator::getConformanceVariable(const Type &type, const Type &protocol,
                                                                      llvm::Constant *conformance) const {
    return new llvm::GlobalVariable(*generator_->module(), generator_->typeHelper().protocolConformance(), true,
                                    llvm::GlobalValue::ExternalLinkage, conformance,
                                    mangleProtocolConformance(type, protocol));
}

}  // namespace EmojicodeCompiler
