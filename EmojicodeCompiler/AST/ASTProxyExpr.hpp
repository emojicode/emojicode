//
//  ASTProxyExpr.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 06/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTProxyExpr_hpp
#define ASTProxyExpr_hpp

#include "ASTExpr.hpp"
#include <functional>
#include <sstream>

namespace EmojicodeCompiler {

class ASTProxyExpr final : public ASTExpr {
public:
    ASTProxyExpr(const SourcePosition &p, const Type &exprType, std::function<llvm::Value*(FnCodeGenerator*)> function)
    : ASTExpr(p), function_(std::move(function)) {
        setExpressionType(exprType);
    }

    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override {
        throw std::logic_error("ASTProxyExpr cannot be analysed.");
    }
    Value* generateExpr(FnCodeGenerator *fncg) const override {
        return function_(fncg);
    }
    void toCode(Prettyprinter &pretty) const override {}
private:
    std::function<llvm::Value*(FnCodeGenerator*)> function_;
};

} // namespace EmojicodeCompiler

#endif /* ASTProxyExpr_hpp */
