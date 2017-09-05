//
//  ASTUnary.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTUnary_hpp
#define ASTUnary_hpp

#include <utility>

#include "../Generation/FnCodeGenerator.hpp"
#include "ASTExpr.hpp"

namespace EmojicodeCompiler {

class ASTUnary : public ASTExpr {
public:
    ASTUnary(std::shared_ptr<ASTExpr> value, const SourcePosition &p) : ASTExpr(p), value_(std::move(value)) {}
protected:
    std::shared_ptr<ASTExpr> value_;
};

class ASTIsNothigness final : public ASTUnary {
    using ASTUnary::ASTUnary;
public:
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generateExpr(FnCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override;
};

class ASTIsError final : public ASTUnary {
    using ASTUnary::ASTUnary;
public:
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generateExpr(FnCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override;
};

class ASTUnwrap final : public ASTUnary {
    using ASTUnary::ASTUnary;
public:
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generateExpr(FnCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    bool error_ = false;
};

class ASTMetaTypeFromInstance final : public ASTUnary {
    using ASTUnary::ASTUnary;
public:
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generateExpr(FnCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override;
};

}  // namespace EmojicodeCompiler

#endif /* ASTUnary_hpp */
