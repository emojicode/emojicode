//
//  CompatibleFunctionParser.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 30/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "CompatibleFunctionParser.hpp"
#include "AST/ASTBinaryOperator.hpp"
#include "AST/ASTVariables.hpp"
#include "CompatibilityInfoProvider.hpp"
#include "Package/Package.hpp"
#include <cassert>

namespace EmojicodeCompiler {

std::shared_ptr<ASTBinaryOperator> CompatibleFunctionParser::parseOperatorCompatibly(OperatorType type,
                                                                           const SourcePosition &position) {
    auto left = parseExpr(10000);
    package_->compatibilityInfoProvider()->nextArgumentsCount(position);
    auto right = parseExpr(10000);
    return std::make_shared<ASTBinaryOperator>(type, left, right, position);
}

std::shared_ptr<ASTExpr> CompatibleFunctionParser::parseExprLeft(const EmojicodeCompiler::Token &token,
                                                                 int precedence) {
    if (token.type() == TokenType::Operator) {
        return parseOperatorCompatibly(operatorType(token.value()), token.position());
    }
    if (token.value().front() == 0x1F38A) {
        return parseOperatorCompatibly(OperatorType::LogicalAndOperator, token.position());
    }
    if (token.value().front() == 0x1F389) {
        return parseOperatorCompatibly(OperatorType::LogicalOrOperator, token.position());
    }
    if (token.value().front() == 0x1F61B) {
        return parseOperatorCompatibly(OperatorType::EqualOperator, token.position());
    }
    return FunctionParser::parseExprLeft(token, precedence);
}

std::shared_ptr<ASTExpr> CompatibleFunctionParser::parseRight(std::shared_ptr<ASTExpr> left, int precendence) {
    return left;
}

std::shared_ptr<ASTExpr> CompatibleFunctionParser::parseClosure(const Token &token) {
    auto selection = package_->compatibilityInfoProvider()->selection();
    auto node = FunctionParser::parseClosure(token);
    package_->compatibilityInfoProvider()->setSelection(selection);
    return node;
}

void CompatibleFunctionParser::parseMainArguments(ASTArguments *arguments, const SourcePosition &position) {
    if (!stream_.nextTokenIs(TokenType::BeginArgumentList)) {
        for (auto i = package_->compatibilityInfoProvider()->nextArgumentsCount(position); i > 0; i--) {
            arguments->addArguments(parseExpr(0));
        }
    }
    else {
        FunctionParser::parseMainArguments(arguments, position);
    }
}

}  // namespace EmojicodeCompiler
