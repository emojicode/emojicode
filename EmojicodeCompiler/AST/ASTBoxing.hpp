//
//  ASTBoxing.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTBoxing_hpp
#define ASTBoxing_hpp

#include "ASTExpr.hpp"

namespace EmojicodeCompiler {

class ASTBoxing : public ASTExpr {
public:
    ASTBoxing(const std::shared_ptr<ASTExpr> &expr, const Type &exprType,
              const SourcePosition &p) : ASTExpr(p), expr_(expr) {
        setExpressionType(exprType);
    }
protected:
    std::shared_ptr<ASTExpr> expr_;
};

class ASTBoxToSimpleOptional final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    void generateExpr(FnCodeGenerator *fncg) const override;
};

class ASTSimpleToSimpleOptional final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    void generateExpr(FnCodeGenerator *fncg) const override;
};

class ASTSimpleOptionalToBox final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    void generateExpr(FnCodeGenerator *fncg) const override;
};

class ASTSimpleToBox final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    void generateExpr(FnCodeGenerator *fncg) const override;
};

class ASTBoxToSimple final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    void generateExpr(FnCodeGenerator *fncg) const override;
};

class ASTDereference final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    void generateExpr(FnCodeGenerator *fncg) const override;
};

class ASTCallableBox final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    void generateExpr(FnCodeGenerator *fncg) const override;
};

class ASTStoreTemporarily final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    void generateExpr(FnCodeGenerator *fncg) const override;
public:
    void setVarId(VariableID id) { varId_ = id; }
private:
    VariableID varId_;
};

}

#endif /* ASTBoxing_hpp */
