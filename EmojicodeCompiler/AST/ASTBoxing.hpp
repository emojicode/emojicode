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
#include <utility>

namespace EmojicodeCompiler {

class BoxingLayer;

class ASTBoxing : public ASTExpr {
public:
    ASTBoxing(std::shared_ptr<ASTExpr> expr, const Type &exprType,
              const SourcePosition &p) : ASTExpr(p), expr_(std::move(expr)) {
        setExpressionType(exprType);
    }
protected:
    std::shared_ptr<ASTExpr> expr_;
    /// Gets a pointer to the value area of box and bit-casts it to the type matching the ASTExpr::expressionType()
    /// of ::expr_.
    /// @param box A pointer to a box.
    Value* getBoxValuePtr(Value *box, FunctionCodeGenerator *fncg) const;
    /// Constructs a simple optional struct according to expressionType() and populates it with value.
    Value* getSimpleOptional(Value *value, FunctionCodeGenerator *fncg) const;
    /// Constructs a simple optional struct according to expressionType() and populates it with value.
    Value* getSimpleOptionalWithoutValue(FunctionCodeGenerator *fncg) const;

    void getPutValueIntoBox(Value *box, Value *value, FunctionCodeGenerator *fncg) const;
    Value* getGetValueFromBox(Value *box, FunctionCodeGenerator *fncg) const;
    /// Allocas space for a box and then calls ASTExpr::generate() of of ::expr_ and stores its value into the box.
    Value* getAllocaTheBox(FunctionCodeGenerator *fncg) const;
};

class ASTBoxToSimpleOptional final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    Value* generate(FunctionCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override {}
};

class ASTSimpleToSimpleOptional final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    Value* generate(FunctionCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override {}
};

class ASTSimpleOptionalToBox final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    Value* generate(FunctionCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override {}
};

class ASTSimpleToBox final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    Value* generate(FunctionCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override {}
};

class ASTBoxToSimple final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    Value* generate(FunctionCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override {}
};

class ASTDereference final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    Value* generate(FunctionCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override {}
};

class ASTCallableBox final : public ASTBoxing {
public:
    ASTCallableBox(std::shared_ptr<ASTExpr> expr, const Type &exprType, const SourcePosition &p,
                   BoxingLayer *boxingLayer) : ASTBoxing(std::move(expr), exprType, p), boxingLayer_(boxingLayer) {}

    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    Value* generate(FunctionCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override {}
private:
    BoxingLayer *boxingLayer_;
};

class ASTStoreTemporarily final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    Value* generate(FunctionCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override {}
public:
    void setVarId(VariableID id) { varId_ = id; }
private:
    VariableID varId_;
};

} // namespace EmojicodeCompiler

#endif /* ASTBoxing_hpp */
