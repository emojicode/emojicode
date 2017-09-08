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
    Value* getBoxValuePtr(Value *box, FunctionCodeGenerator *fg) const;
    /// Constructs a simple optional struct according to expressionType() and populates it with value.
    Value* getSimpleOptional(Value *value, FunctionCodeGenerator *fg) const;
    /// Constructs a simple optional struct according to expressionType() and populates it with value.
    Value* getSimpleOptionalWithoutValue(FunctionCodeGenerator *fg) const;

    void getPutValueIntoBox(Value *box, Value *value, FunctionCodeGenerator *fg) const;
    Value* getGetValueFromBox(Value *box, FunctionCodeGenerator *fg) const;
    /// Allocas space for a box and then calls ASTExpr::generate() of of ::expr_ and stores its value into the box.
    Value* getAllocaTheBox(FunctionCodeGenerator *fg) const;
};

class ASTBoxToSimpleOptional final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override {}
};

class ASTSimpleToSimpleOptional final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override {}
};

class ASTSimpleOptionalToBox final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override {}
};

class ASTSimpleToBox final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override {}
};

class ASTBoxToSimple final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override {}
};

class ASTDereference final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override {}
};

class ASTCallableBox final : public ASTBoxing {
public:
    ASTCallableBox(std::shared_ptr<ASTExpr> expr, const Type &exprType, const SourcePosition &p,
                   BoxingLayer *boxingLayer) : ASTBoxing(std::move(expr), exprType, p), boxingLayer_(boxingLayer) {}

    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override {}
private:
    BoxingLayer *boxingLayer_;
};

class ASTStoreTemporarily final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
public:
    /// This setter must be called to indicate that the value that is temporarily stored by this node is the result
    /// of a value type initialization. If called, the contained expression will be treated as a ASTInitialization
    /// node and an address to the reserved space is passed to ASTInitialization::setDestination() upon generation.
    /// @see SemanticAnalyser::comply()
    void setInit() { init_ = true; }
    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override {}
private:
    bool init_ = false;
};

} // namespace EmojicodeCompiler

#endif /* ASTBoxing_hpp */
