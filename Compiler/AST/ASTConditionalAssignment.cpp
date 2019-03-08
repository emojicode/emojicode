//
//  ASTConditionalAssignment.cpp
//  runtime
//
//  Created by Theo Weidmann on 07.03.19.
//

#include "ASTConditionalAssignment.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "CompilerError.hpp"
#include "Analysis/ExpressionAnalyser.hpp"
#include "MemoryFlowAnalysis/MFFunctionAnalyser.hpp"
#include "Types/TypeExpectation.hpp"
#include <llvm/IR/Value.h>
#include "Scoping/SemanticScoper.hpp"

namespace EmojicodeCompiler {

Value* ASTConditionalAssignment::generate(FunctionCodeGenerator *fg) const {
    auto optional = expr_->generate(fg);

    if (expr_->expressionType().type() == TypeType::Box) {
        fg->setVariable(varId_, optional);
        auto vf = fg->builder().CreateExtractValue(optional, 0);
        return fg->builder().CreateICmpNE(vf, llvm::Constant::getNullValue(vf->getType()));
    }

    auto value = fg->buildGetOptionalValue(optional, expr_->expressionType());
    fg->setVariable(varId_, value);
    return fg->buildOptionalHasValue(optional, expr_->expressionType());
}

Type ASTConditionalAssignment::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    Type t = analyser->expect(TypeExpectation(false, false), &expr_);
    if (t.unboxedType() != TypeType::Optional) {
        throw CompilerError(position(), "Condition assignment can only be used with optionals.");
    }

    t = t.optionalType();

    auto &variable = analyser->scoper().currentScope().declareVariable(varName_, t, true, position());
    analyser->pathAnalyser().record(PathAnalyserIncident(false, variable.id()));
    varId_ = variable.id();

    return analyser->boolean();
}

void ASTConditionalAssignment::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    analyser->recordVariableSet(varId_, expr_.get(), expr_->expressionType().optionalType());
    analyser->take(expr_.get());
}

}
