//
//  ASTLiterals_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTLiterals.hpp"
#include "ASTInitialization.hpp"
#include "Compiler.hpp"
#include "Generation/CallCodeGenerator.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "Generation/Declarator.hpp"
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

Value* ASTSymbolLiteral::generate(FunctionCodeGenerator *fg) const {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(fg->generator()->context()), value_);
}

Value* ASTThis::generate(FunctionCodeGenerator *fg) const {
    if (!isTemporary()) {
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
    auto init = type_.typeDefinition()->lookupInitializer({ 0x1F438 });
    auto dict = ASTInitialization::initObject(fg, ASTArguments(position()), init, type_);
    for (auto it = values_.begin(); it != values_.end(); it++) {
        auto args = ASTArguments(position());
        args.addArguments(*it);
        args.addArguments(*(++it));
        auto method = type_.typeDefinition()->lookupMethod({ 0x1F437 }, true);
        CallCodeGenerator(fg, CallType::StaticDispatch).generate(dict, type_, args, method);
    }
    return handleResult(fg, dict);
}

Value* ASTListLiteral::generate(FunctionCodeGenerator *fg) const {
    auto init = type_.typeDefinition()->lookupInitializer({ 0x1F438 });
    auto list = ASTInitialization::initObject(fg, ASTArguments(position()), init, type_);
    for (auto &value : values_) {
        auto args = ASTArguments(position());
        args.addArguments(value);
        auto method = type_.typeDefinition()->lookupMethod({ 0x1F43B }, true);
        CallCodeGenerator(fg, CallType::StaticDispatch).generate(list, type_, args, method);
    }
    return handleResult(fg, list);
}

Value* ASTConcatenateLiteral::generate(FunctionCodeGenerator *fg) const {
    auto init = type_.typeDefinition()->lookupInitializer({ 0x1F195 });
    auto strbuilder = ASTInitialization::initObject(fg, ASTArguments(position()), init, type_, true);
    for (auto &stringNode : values_) {
        auto args = ASTArguments(position());
        args.addArguments(stringNode);
        auto method = type_.typeDefinition()->lookupMethod({ 0x1F43B }, true);
        CallCodeGenerator(fg, CallType::StaticDispatch).generate(strbuilder, type_, args, method);
    }
    auto method = type_.typeDefinition()->lookupMethod({ 0x1F521 }, true);
    auto str = CallCodeGenerator(fg, CallType::StaticDispatch).generate(strbuilder, type_,
                                                                        ASTArguments(position()), method);
    fg->release(strbuilder, type_);
    return handleResult(fg, str);
}

}  // namespace EmojicodeCompiler
