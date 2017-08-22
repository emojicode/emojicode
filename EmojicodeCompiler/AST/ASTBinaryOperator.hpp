//
//  ASTBinaryOperator.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTBinaryOperator_hpp
#define ASTBinaryOperator_hpp

#include "ASTMethod.hpp"

namespace EmojicodeCompiler {

class ASTBinaryOperator : public ASTMethodable {
public:
    ASTBinaryOperator(OperatorType op, const std::shared_ptr<ASTExpr> &left, const std::shared_ptr<ASTExpr> &right,
                      const SourcePosition &p) : ASTMethodable(p), operator_(op), left_(left), right_(right) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
private:
    struct BuiltIn {
        BuiltIn(Type type) : returnType(type), swap(false) {}
        BuiltIn(Type type, bool swap) : returnType(type), swap(swap) {}
        Type returnType;
        bool swap;
    };

    std::pair<bool, BuiltIn> builtInPrimitiveOperator(SemanticAnalyser *analyser, const Type &type);

    bool builtIn_ = false;
    OperatorType operator_;
    std::shared_ptr<ASTExpr> left_;
    std::shared_ptr<ASTExpr> right_;
};

}

#endif /* ASTBinaryOperator_hpp */
