//
//  CompatibleFunctionParser.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 30/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef CompatibleFunctionParser_hpp
#define CompatibleFunctionParser_hpp

#include "FunctionParser.hpp"

namespace EmojicodeCompiler {

/// This subclass of FunctionParser can parse Emojicode 0.5 $arguments$ and will parse Emojicode 0.5 method calls
/// that can be converted to operators to as operation.
class CompatibleFunctionParser : public FunctionParser {
public:
   using FunctionParser::FunctionParser;
private:
    std::shared_ptr<ASTExpr> parseExprLeft(const EmojicodeCompiler::Token &token, int precedence) override;
    std::shared_ptr<ASTExpr> parseRight(std::shared_ptr<ASTExpr> left, int precendence) override;
    std::shared_ptr<ASTStatement> parseVariableAssignment(const Token &token) override;
    std::shared_ptr<ASTBinaryOperator> parseOperatorCompatibly(OperatorType type, const SourcePosition &position);

    void parseMainArguments(ASTArguments *arguments) override;
};

}  // namespace EmojicodeCompiler

#endif /* CompatibleFunctionParser_hpp */
