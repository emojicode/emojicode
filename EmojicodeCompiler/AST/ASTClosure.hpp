//
//  ASTClosure.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 16/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTClosure_hpp
#define ASTClosure_hpp

#include "../Scoping/CapturingSemanticScoper.hpp"
#include "ASTExpr.hpp"

namespace EmojicodeCompiler {

class ASTClosure : public ASTExpr {
public:
    ASTClosure(Function *function, const SourcePosition &p) : ASTExpr(p), function_(function) {}

    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    Function *function_;
    std::vector<VariableCapture> captures_;
    bool captureSelf_ = false;
};

}  // namespace EmojicodeCompiler

#endif /* ASTClosure_hpp */
