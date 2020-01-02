//
//  ASTConditionalAssignment.hpp
//  runtime
//
//  Created by Theo Weidmann on 07.03.19.
//

#ifndef ASTConditionalAssignment_hpp
#define ASTConditionalAssignment_hpp

#include "ASTExpr.hpp"
#include "Scoping/Variable.hpp"

namespace EmojicodeCompiler {

class ASTConditionalAssignment final : public ASTExpr {
public:
    ASTConditionalAssignment(std::u32string varName, std::shared_ptr<ASTExpr> expr,
                             const SourcePosition &p) : ASTExpr(p), varName_(std::move(varName)), expr_(std::move(expr)) {}
    Type analyse(ExpressionAnalyser *analyser) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override;

private:
    std::u32string varName_;
    std::shared_ptr<ASTExpr> expr_;
    VariableID varId_;
};

}

#endif /* ASTConditionalAssignment_hpp */
