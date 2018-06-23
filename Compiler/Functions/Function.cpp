//
//  Function.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Function.hpp"
#include "Compiler.hpp"
#include "Types/TypeContext.hpp"
#include "Types/TypeDefinition.hpp"
#include "Types/TypeContext.hpp"
#include "AST/ASTStatements.hpp"
#include <llvm/IR/Function.h>

namespace EmojicodeCompiler {

Function::Function(std::u32string name, AccessLevel level, bool final, TypeDefinition *owner, Package *package,
         SourcePosition p,
         bool overriding, std::u32string documentationToken, bool deprecated, bool mutating, bool imperative,
         bool unsafe, FunctionType type) :
position_(std::move(p)), name_(std::move(name)), final_(final), overriding_(overriding),
deprecated_(deprecated), imperative_(imperative), unsafe_(unsafe), mutating_(mutating), access_(level),
owner_(owner), package_(package), documentation_(std::move(documentationToken)),
functionType_(type) {}

llvm::FunctionType* FunctionReification::functionType() {
    if (functionType_ != nullptr) {
        return functionType_;
    }
    assert(function != nullptr);
    return function->getFunctionType();
}

TypeContext Function::typeContext() {
    auto type = owner_ == nullptr ? Type::noReturn() : owner_->type();
    if (type.type() == TypeType::ValueType || type.type() == TypeType::Enum) {
        type.setReference();
    }
    if (functionType() == FunctionType::ClassMethod) {
        type = Type(MakeTypeAsValue, type);
    }
    return TypeContext(type.applyMinimalBoxing(), this);
}

Function::~Function() = default;

void Function::setAst(std::unique_ptr<ASTBlock> ast) {
    ast_ = std::move(ast);
}

}  // namespace EmojicodeCompiler
