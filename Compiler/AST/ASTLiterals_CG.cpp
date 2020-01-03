//
//  ASTLiterals_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include <utility>
#include "ASTInitialization.hpp"
#include "ASTLiterals.hpp"
#include "Generation/TypeDescriptionGenerator.hpp"
#include "Compiler.hpp"
#include "Generation/CallCodeGenerator.hpp"
#include "Generation/RunTimeHelper.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "Generation/StringPool.hpp"
#include "Types/Class.hpp"

namespace EmojicodeCompiler {

Value* ASTStringLiteral::generate(FunctionCodeGenerator *fg) const {
    return fg->generator()->stringPool().pool(value_);
}

Value* ASTCGUTF8Literal::generate(FunctionCodeGenerator *fg) const {
    return fg->generator()->stringPool().addToPool(value_);
}

Value* ASTBooleanTrue::generate(FunctionCodeGenerator *fg) const {
    return llvm::ConstantInt::getTrue(fg->ctx());
}

Value* ASTBooleanFalse::generate(FunctionCodeGenerator *fg) const {
    return llvm::ConstantInt::getFalse(fg->ctx());
}

Value* ASTNumberLiteral::generate(FunctionCodeGenerator *fg) const {
    switch (type_) {
        case NumberType::Byte:
            return fg->int8(integerValue_);
        case NumberType::Integer:
            return fg->int64(integerValue_);
        case NumberType::Double:
            return llvm::ConstantFP::get(llvm::Type::getDoubleTy(fg->ctx()), doubleValue_);
    }
}

Value* ASTThis::generate(FunctionCodeGenerator *fg) const {
    if (expressionType().type() == TypeType::Class && !isTemporary()) {
        // Only for class objects. For value types, $this$ is a reference, which is retained when dereferenced.
        fg->retain(fg->thisValue(), expressionType());
    }
    return fg->thisValue();
}

Value* ASTNoValue::generate(FunctionCodeGenerator *fg) const {
    if (type_.storageType() == StorageType::Box) {
        return fg->buildBoxWithoutValue();
    }
    return fg->buildSimpleOptionalWithoutValue(type_);
}

Value* ASTCollectionLiteral::init(FunctionCodeGenerator *fg, std::vector<llvm::Value*> args) const {
    auto td = TypeDescriptionGenerator(fg, TypeDescriptionUser::ValueTypeOrValue).generate(type_.genericArguments());
    auto value = fg->createEntryAlloca(fg->typeHelper().llvmTypeFor(type_));
    args.emplace_back(td);
    CallCodeGenerator(fg, CallType::StaticDispatch).generate(value, type_, ASTArguments(position()),
                                                             initializer_, nullptr, args);
    handleResult(fg, nullptr, value);
    return fg->builder().CreateLoad(value);
}

std::pair<llvm::Value *, llvm::Value *> EmojicodeCompiler::ASTCollectionLiteral::prepareValueArray(FunctionCodeGenerator *fg, llvm::Type *type, size_t count,
                                                                                                   const char *name) const {
    auto arrayType = llvm::ArrayType::get(type, count);
    auto structType = llvm::StructType::get(fg->generator()->context(),
                                            { fg->builder().getInt8PtrTy(), arrayType });

    auto structure = fg->createEntryAlloca(structType, name);
    fg->builder().CreateStore(fg->generator()->runTime().ignoreBlockPtr(),
                              fg->builder().CreateConstInBoundsGEP2_32(structType, structure, 0, 0));
    auto values = fg->builder().CreateConstInBoundsGEP2_32(structType, structure, 0, 1);
    return std::make_pair(fg->builder().CreateConstInBoundsGEP2_32(arrayType, values, 0, 0), structure);
}


Value* ASTCollectionLiteral::generate(FunctionCodeGenerator *fg) const {
    if (pairs_) return generatePairs(fg);
    llvm::Value *current, *structure;
    std::tie(current, structure) = prepareValueArray(fg, fg->typeHelper().box(), values_.size(), "items");
    for (auto &value : values_) {
        fg->builder().CreateStore(value->generate(fg), current);
        current = fg->builder().CreateConstInBoundsGEP1_32(fg->typeHelper().box(), current, 1);
    }
    return init(fg, { fg->builder().CreateBitCast(structure, fg->builder().getInt8PtrTy()),
                      fg->int64(values_.size()) });
}

Value *ASTCollectionLiteral::generatePairs(FunctionCodeGenerator *fg) const {
    llvm::Value *keys, *values, *currentKey, *currentValue;
    auto string = fg->typeHelper().llvmTypeFor(Type(fg->compiler()->sString));
    std::tie(currentKey, keys) = prepareValueArray(fg, string, values_.size() / 2, "keys");
    std::tie(currentValue, values) = prepareValueArray(fg, fg->typeHelper().box(), values_.size() / 2, "values");
    auto it = values_.begin();
    while (it != values_.end()) {
        fg->builder().CreateStore((*it++)->generate(fg), currentKey);
        fg->builder().CreateStore((*it++)->generate(fg), currentValue);
        currentKey = fg->builder().CreateConstInBoundsGEP1_32(string, currentKey, 1);
        currentValue = fg->builder().CreateConstInBoundsGEP1_32(fg->typeHelper().box(), currentValue, 1);
    }
    return init(fg, {fg->builder().CreateBitCast(keys, fg->builder().getInt8PtrTy()),
                     fg->builder().CreateBitCast(values, fg->builder().getInt8PtrTy()), fg->int64(values_.size() / 2)});
}


Value* ASTInterpolationLiteral::generate(FunctionCodeGenerator *fg) const {
    int64_t length = 0;
    for (auto &literal : literals_) {
        length += literal.size() + 6;
    }
    auto type = init_->owner()->type();
    auto lengthNode = std::make_shared<ASTNumberLiteral>(length, U"", position());
    auto builder = ASTInitialization::initObject(fg, ASTArguments(position(), {lengthNode}), init_, type, nullptr, true,
                                                 nullptr);

    auto literalsIt = literals_.begin();
    append(fg, *literalsIt++, builder);
    for (auto &value : values_) {
        auto str = CallCodeGenerator(fg, CallType::DynamicProtocolDispatch).generate(value->generate(fg), value->expressionType(),
                                                                 ASTArguments(position()), toString_, nullptr);
        append(fg, str, builder);
        append(fg, *literalsIt++, builder);
    }

    auto str = CallCodeGenerator(fg, CallType::StaticDispatch).generate(builder, type,
                                                                        ASTArguments(position()), get_, nullptr);
    fg->release(builder, type);
    return handleResult(fg, str);
}

void
ASTInterpolationLiteral::append(FunctionCodeGenerator *fg, llvm::Value *value, llvm::Value *builder) const {
    CallCodeGenerator(fg, CallType::StaticDispatch).generate(builder, init_->owner()->type(), ASTArguments(position()),
                                                             append_, nullptr,
                                                             {value});
}

void
ASTInterpolationLiteral::append(FunctionCodeGenerator *fg, const std::u32string &literal, llvm::Value *builder) const {
    if (literal.empty()) {
        return;
    }
    append(fg, fg->generator()->stringPool().pool(literal), builder);
}

}  // namespace EmojicodeCompiler
