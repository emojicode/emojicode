//
//  ASTInitialization.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTInitialization_hpp
#define ASTInitialization_hpp

#include <utility>

#include "ASTExpr.hpp"

namespace EmojicodeCompiler {

class ASTVTInitDest final : public ASTExpr {
public:
    ASTVTInitDest(VariableID varId, bool inInstanceScope, bool declare, const Type &exprType, const SourcePosition &p)
    : ASTExpr(p), inInstanceScope_(inInstanceScope), varId_(varId), declare_(declare) {
        setExpressionType(exprType);
    }

    Value* generateExpr(FnCodeGenerator *fncg) const override;
    void initialize(FnCodeGenerator *fncg);
    void toCode(Prettyprinter &pretty) const override {}

    Type analyse(SemanticAnalyser *, const TypeExpectation &) override { return expressionType(); }
private:
    CGScoper& scoper(FnCodeGenerator *fncg) const;

    bool inInstanceScope_;
    VariableID varId_;
    bool declare_;
};

class ASTInitialization final : public ASTExpr {
public:
    enum class InitType {
        Enum, ValueType, Class
    };

    ASTInitialization(std::u32string name, std::shared_ptr<ASTTypeExpr> type,
                      ASTArguments args, const SourcePosition &p)
    : ASTExpr(p), name_(std::move(name)), typeExpr_(std::move(type)), args_(std::move(args)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generateExpr(FnCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override;

    void setDestination(const std::shared_ptr<ASTVTInitDest> &vtInitable) { vtDestination_ = vtInitable; }
    InitType initType() { return initType_; }
private:
    InitType initType_ = InitType::Class;
    std::u32string name_;
    std::shared_ptr<ASTTypeExpr> typeExpr_;
    std::shared_ptr<ASTVTInitDest> vtDestination_;
    ASTArguments args_;
};

}  // namespace EmojicodeCompiler

#endif /* ASTInitialization_hpp */
