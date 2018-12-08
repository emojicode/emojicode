//
//  ASTLiterals_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTInitialization.hpp"
#include "ASTLiterals.hpp"
#include "Compiler.hpp"
#include "Generation/CallCodeGenerator.hpp"
#include "Generation/Declarator.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "Generation/StringPool.hpp"
#include "Types/Class.hpp"

namespace EmojicodeCompiler {

Value* ASTStringLiteral::generate(FunctionCodeGenerator *fg) const {
    return fg->generator()->stringPool().pool(value_);
}

Value* ASTBooleanTrue::generate(FunctionCodeGenerator *fg) const {
    return llvm::ConstantInt::getTrue(fg->generator()->context());
}

Value* ASTBooleanFalse::generate(FunctionCodeGenerator *fg) const {
    return llvm::ConstantInt::getFalse(fg->generator()->context());
}

Value* ASTNumberLiteral::generate(FunctionCodeGenerator *fg) const {
    switch (type_) {
        case NumberType::Byte:
            return fg->int8(integerValue_);
        case NumberType::Integer:
            return fg->int64(integerValue_);
        case NumberType::Double:
            return llvm::ConstantFP::get(llvm::Type::getDoubleTy(fg->generator()->context()), doubleValue_);
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
        return fg->buildBoxOptionalWithoutValue();
    }
    return fg->buildSimpleOptionalWithoutValue(type_);
}

Value* ASTDictionaryLiteral::generate(FunctionCodeGenerator *fg) const {
    auto init = type_.typeDefinition()->lookupInitializer(U"🐴");
    auto capacity = std::make_shared<ASTNumberLiteral>(static_cast<int64_t>(values_.size() / 2), U"", position());

    auto dict = fg->createEntryAlloca(fg->typeHelper().llvmTypeFor(type_));
    CallCodeGenerator(fg, CallType::StaticDispatch).generate(dict, type_, ASTArguments(position(), { capacity }), init);
    for (auto it = values_.begin(); it != values_.end(); it++) {
        auto key = *(it++);
        auto args = ASTArguments(position(), { *it, key });
        auto method = type_.typeDefinition()->lookupMethod(U"🐽", Mood::Assignment);
        CallCodeGenerator(fg, CallType::StaticDispatch).generate(dict, type_, args, method);
    }
    handleResult(fg, nullptr, dict);
    return fg->builder().CreateLoad(dict);
}

Value* ASTListLiteral::generate(FunctionCodeGenerator *fg) const {
    auto init = type_.typeDefinition()->lookupInitializer(U"🐴");
    auto capacity = std::make_shared<ASTNumberLiteral>(static_cast<int64_t>(values_.size()), U"", position());

    auto list = fg->createEntryAlloca(fg->typeHelper().llvmTypeFor(type_));
    CallCodeGenerator(fg, CallType::StaticDispatch).generate(list, type_, ASTArguments(position(), { capacity }), init);
    for (auto &value : values_) {
        auto args = ASTArguments(position(), { value });
        auto method = type_.typeDefinition()->lookupMethod(U"🐻", Mood::Imperative);
        CallCodeGenerator(fg, CallType::StaticDispatch).generate(list, type_, args, method);
    }
    handleResult(fg, nullptr, list);
    return fg->builder().CreateLoad(list);
}

Value* ASTConcatenateLiteral::generate(FunctionCodeGenerator *fg) const {
    auto init = type_.typeDefinition()->lookupInitializer(U"🔡");

    auto it = values_.begin();
    auto builder = ASTInitialization::initObject(fg, ASTArguments(position(), { *it++ }), init, type_, true);

    for (auto end = values_.end(); it != end; it++) {
        auto args = ASTArguments(position(), { *it });
        auto method = type_.typeDefinition()->lookupMethod({ 0x1F43B }, Mood::Imperative);
        CallCodeGenerator(fg, CallType::StaticDispatch).generate(builder, type_, args, method);
    }
    auto method = type_.typeDefinition()->lookupMethod({ 0x1F521 }, Mood::Imperative);
    auto str = CallCodeGenerator(fg, CallType::StaticDispatch).generate(builder, type_,
                                                                        ASTArguments(position()), method);
    fg->release(builder, type_);
    return handleResult(fg, str);
}

}  // namespace EmojicodeCompiler
