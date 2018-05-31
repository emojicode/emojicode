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
    return fg->thisValue();
}

Value* ASTNoValue::generate(FunctionCodeGenerator *fg) const {
    if (type_.storageType() == StorageType::Box) {
        return fg->getBoxOptionalWithoutValue();
    }
    return fg->getSimpleOptionalWithoutValue(type_);
}

Value* ASTDictionaryLiteral::generate(FunctionCodeGenerator *fg) const {
    auto dict = ASTInitialization::initObject(fg, ASTArguments(position()), std::u32string(1, 0x1F438), type_);
    for (auto it = values_.begin(); it != values_.end(); it++) {
        auto args = ASTArguments(position());
        args.addArguments(*it);
        args.addArguments(*(++it));
        CallCodeGenerator(fg, CallType::StaticDispatch).generate(dict, type_, args, std::u32string(1, 0x1F437));
    }
    return dict;
}

Value* ASTListLiteral::generate(FunctionCodeGenerator *fg) const {
    auto list = ASTInitialization::initObject(fg, ASTArguments(position()), std::u32string(1, 0x1F438), type_);
    for (auto &value : values_) {
        auto args = ASTArguments(position());
        args.addArguments(value);
        CallCodeGenerator(fg, CallType::StaticDispatch).generate(list, type_, args, std::u32string(1, 0x1F43B));
    }
    return list;
}

Value* ASTConcatenateLiteral::generate(FunctionCodeGenerator *fg) const {
    auto strbuilder = ASTInitialization::initObject(fg, ASTArguments(position()), std::u32string(1, 0x1F195), type_);
    for (auto &stringNode : values_) {
        auto args = ASTArguments(position());
        args.addArguments(stringNode);
        CallCodeGenerator(fg, CallType::StaticDispatch).generate(strbuilder, type_, args, std::u32string(1, 0x1F43B));
    }
    return CallCodeGenerator(fg, CallType::StaticDispatch).generate(strbuilder, type_, ASTArguments(position()),
                                                                    std::u32string(1, 0x1F521));
}

}  // namespace EmojicodeCompiler
