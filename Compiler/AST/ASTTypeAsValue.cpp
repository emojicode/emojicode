//
//  ASTTypeAsValue.cpp
//  runtime
//
//  Created by Theo Weidmann on 07.03.19.
//

#include "Analysis/ExpressionAnalyser.hpp"
#include "ASTTypeAsValue.hpp"
#include "ASTType.hpp"
#include "Types/Class.hpp"
#include "Generation/FunctionCodeGenerator.hpp"

namespace EmojicodeCompiler {

Type ASTTypeAsValue::analyse(ExpressionAnalyser *analyser) {
    auto &type = type_->analyseType(analyser->typeContext());
    ASTTypeValueType::checkTypeValue(tokenType_, type, analyser->typeContext(), position(), analyser->package());
    return Type(MakeTypeAsValue, type);
}

Value* ASTTypeAsValue::generate(FunctionCodeGenerator *fg) const {
    if (type_->type().type() == TypeType::Class) {
        return type_->type().klass()->classInfo();
    }
    return llvm::UndefValue::get(fg->typeHelper().llvmTypeFor(Type(MakeTypeAsValue, type_->type())));
}

ASTTypeAsValue::ASTTypeAsValue(std::unique_ptr<ASTType> type, TokenType tokenType, const SourcePosition &p)
    : ASTExpr(p), type_(std::move(type)), tokenType_(tokenType) {}
ASTTypeAsValue::~ASTTypeAsValue() = default;

}  // namespace EmojicodeCompiler
