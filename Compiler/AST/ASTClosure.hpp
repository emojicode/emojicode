//
//  ASTClosure.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 16/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTClosure_hpp
#define ASTClosure_hpp

#include "Scoping/CapturingSemanticScoper.hpp"
#include "ASTExpr.hpp"

namespace EmojicodeCompiler {

class ASTClosure : public ASTExpr {
public:
    ASTClosure(std::unique_ptr<Function> &&closure, const SourcePosition &p);

    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override final;
    void toCode(Prettyprinter &pretty) const override;
private:
    std::unique_ptr<Function> closure_;
    std::vector<VariableCapture> captures_;
    bool captureSelf_ = false;
};

}  // namespace EmojicodeCompiler

#endif /* ASTClosure_hpp */
