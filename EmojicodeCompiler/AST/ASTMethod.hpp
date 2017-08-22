//
//  ASTMethod.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTMethod_hpp
#define ASTMethod_hpp

#include "ASTExpr.hpp"

namespace EmojicodeCompiler {

class SemanticAnalyser;

class ASTMethodable : public ASTExpr {
protected:
    ASTMethodable(const SourcePosition &p) : ASTExpr(p), args_(p) {}
    ASTMethodable(const SourcePosition &p, const ASTArguments &args) : ASTExpr(p), args_(args) {}
    Type analyseMethodCall(SemanticAnalyser *analyser, const EmojicodeString &name,
                           std::shared_ptr<ASTExpr> &callee);
    EmojicodeInstruction instruction_;
    ASTArguments args_;
    bool invert_ = false;
};

class ASTMethod final : public ASTMethodable {
public:
    ASTMethod(const EmojicodeString &name, const std::shared_ptr<ASTExpr> &callee,
              const ASTArguments &args, const SourcePosition &p)
    : ASTMethodable(p, args), name_(name), callee_(callee) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
private:
    void generateExpr(FnCodeGenerator *fncg) const override;
    EmojicodeString name_;
    std::shared_ptr<ASTExpr> callee_;
};

}

#endif /* ASTMethod_hpp */
