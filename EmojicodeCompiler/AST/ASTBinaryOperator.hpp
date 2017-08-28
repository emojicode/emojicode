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
    void generateExpr(FnCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    struct BuiltIn {
        explicit BuiltIn(Type type) : returnType(std::move(type)), swap(false) {}
        BuiltIn(Type type, bool swap) : returnType(std::move(type)), swap(swap) {}
        Type returnType;
        bool swap;
    };

    std::pair<bool, BuiltIn> builtInPrimitiveOperator(SemanticAnalyser *analyser, const Type &type);

    bool builtIn_ = false;
    OperatorType operator_;
    std::shared_ptr<ASTExpr> left_;
    std::shared_ptr<ASTExpr> right_;
};

}  // namespace EmojicodeCompiler

#endif /* ASTBinaryOperator_hpp */
