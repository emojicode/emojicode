//
//  Function.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Function.hpp"
#include "AST/ASTStatements.hpp"
#include "Compiler.hpp"
#include "Types/TypeContext.hpp"
#include "Types/TypeDefinition.hpp"
#include <llvm/IR/Function.h>
#include "AST/ASTStatements.hpp"

namespace EmojicodeCompiler {

Function::Function(std::u32string name, AccessLevel level, bool final, TypeDefinition *owner, Package *package,
                   SourcePosition p, bool overriding, std::u32string documentationToken, bool deprecated, bool mutating,
                   Mood mood, bool unsafe, FunctionType type, bool forceInline) :
Definition(package, std::move(documentationToken), std::move(name), p), final_(final), deprecated_(deprecated),
mood_(mood), unsafe_(unsafe), forceInline_(forceInline), mutating_(mutating),
superFunction_(overriding ? this : nullptr),  access_(level), owner_(owner), functionType_(type) {}

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
        type.setMutable(mutating());
    }
    if (isTypeMethod(this)) {
        type = Type(MakeTypeAsValue, type);
    }
    return TypeContext(type.applyMinimalBoxing(), this);
}

bool Function::isInline() const {
    return forceInline_ || (ast() != nullptr && ast()->stmtsSize() <= 2 &&
                            functionType() != FunctionType::Deinitializer &&
                            functionType() != FunctionType::CopyRetainer);
}

Function::~Function() = default;

void Function::setAst(std::unique_ptr<ASTBlock> ast) {
    ast_ = std::move(ast);
}

}  // namespace EmojicodeCompiler
