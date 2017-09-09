//
//  ASTBinaryOperator.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTBinaryOperator_hpp
#define ASTBinaryOperator_hpp

#include "../Parsing/OperatorHelper.hpp"
#include "ASTMethod.hpp"
#include <utility>

namespace EmojicodeCompiler {

class ASTBinaryOperator : public ASTMethodable {
public:
    ASTBinaryOperator(OperatorType op, std::shared_ptr<ASTExpr> left, std::shared_ptr<ASTExpr> right,
                      const SourcePosition &p) : ASTMethodable(p), operator_(op), left_(std::move(left)), right_(std::move(right)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    
    struct BuiltIn {
        explicit BuiltIn(Type type) : returnType(std::move(type)) {}
        Type returnType;
    };

    std::pair<bool, BuiltIn> builtInPrimitiveOperator(SemanticAnalyser *analyser, const Type &type);
    void printBinaryOperand(int precedence, const std::shared_ptr<ASTExpr> &expr, Prettyprinter &pretty) const;

    OperatorType operator_;
    std::shared_ptr<ASTExpr> left_;
    std::shared_ptr<ASTExpr> right_;
};

}  // namespace EmojicodeCompiler

#endif /* ASTBinaryOperator_hpp */
