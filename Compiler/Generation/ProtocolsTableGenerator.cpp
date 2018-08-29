//
// Created by Theo Weidmann on 07.02.18.
//

#include "ProtocolsTableGenerator.hpp"
#include "Functions/Function.hpp"
#include "Generation/Mangler.hpp"
#include "LLVMTypeHelper.hpp"
#include "Types/Class.hpp"
#include "Types/ValueType.hpp"
#include "Types/Protocol.hpp"
#include "Types/Type.hpp"
#include "Types/TypeDefinition.hpp"
#include "CodeGenerator.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>

namespace EmojicodeCompiler {

BoxInfoGenerator::BoxInfoGenerator(const Type &type, CodeGenerator *generator) : generator_(generator), type_(type) {
    boxInfos_.reserve(type.typeDefinition()->protocols().size());
    boxInfo_ = generator_->declarator().declareBoxInfo(mangleBoxInfoName(type_),
                                                       type.typeDefinition()->protocols().size());
}

void BoxInfoGenerator::add(const Type &protocol, llvm::GlobalVariable *conformance) {
    boxInfos_.emplace_back(llvm::ConstantStruct::get(generator_->typeHelper().boxInfo(), {
        generator_->protocolIdentifierFor(protocol), conformance
    }));
}

void BoxInfoGenerator::finish() {
    generator_->declarator().initBoxInfo(boxInfo_, std::move(boxInfos_));
    type_.unboxed().typeDefinition()->setBoxInfo(boxInfo_);
}

void ProtocolsTableGenerator::createProtocolsTable(const Type &type) {
    std::map<Type, llvm::Constant *> tables;
    BoxInfoGenerator big(type, generator_);
    auto boxInfo = type.type() == TypeType::Class ? generator_->declarator().boxInfoForObjects() : big.boxInfo();

    for (auto &protocol : type.typeDefinition()->protocols()) {
        auto conformance = createVirtualTable(type, protocol->type(), boxInfo);
        tables.emplace(protocol->type().unboxed(), conformance);
        big.add(protocol->type(), conformance);
    }

    big.finish();
    type.typeDefinition()->setProtocolTables(std::move(tables));
}

void ProtocolsTableGenerator::declareImportedProtocolsTable(const Type &type) {
    std::map<Type, llvm::Constant *> tables;
    BoxInfoGenerator big(type, generator_);

    for (auto &protocol : type.typeDefinition()->protocols()) {
        auto conformance = getConformanceVariable(type, protocol->type(), nullptr);
        tables.emplace(protocol->type().unboxed(), conformance);
        big.add(protocol->type(), conformance);
    }

    big.finish();
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

llvm::GlobalVariable* ProtocolsTableGenerator::createVirtualTable(const Type &type, const Type &protocol,
                                                                  llvm::Constant *boxInfo) {
    auto typeDef = type.typeDefinition();
    auto arrayType = llvm::ArrayType::get(llvm::Type::getInt8PtrTy(generator_->context()),
                                          protocol.protocol()->methodList().size());

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
    auto arrayVar = new llvm::GlobalVariable(*generator_->module(), arrayType, true,
                                             llvm::GlobalValue::LinkageTypes::PrivateLinkage, array);
    auto load = llvm::ConstantInt::get(llvm::Type::getInt1Ty(generator_->context()),
                                       (type.type() == TypeType::Class ||
                                        generator_->typeHelper().isRemote(type)) ? 1 : 0);
    auto conformance = llvm::ConstantStruct::get(generator_->typeHelper().protocolConformance(),
                                                 {load, arrayVar, llvm::ConstantExpr::getBitCast(boxInfo, generator_->typeHelper().boxInfo()->getPointerTo()) });
    return getConformanceVariable(type, protocol, conformance);
}

llvm::GlobalVariable* ProtocolsTableGenerator::getConformanceVariable(const Type &type, const Type &protocol,
                                                                      llvm::Constant *conformance) const {
    return new llvm::GlobalVariable(*generator_->module(), generator_->typeHelper().protocolConformance(), true,
                                    llvm::GlobalValue::ExternalLinkage, conformance,
                                    mangleProtocolConformance(type, protocol));
}

}  // namespace EmojicodeCompiler
