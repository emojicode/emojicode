//
//  ASTClosure.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 16/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTClosure_hpp
#define ASTClosure_hpp

#include "ASTExpr.hpp"
#include "Scoping/CapturingSemanticScoper.hpp"
#include "MemoryFlowAnalysis/MFHeapAllocates.hpp"
#include <llvm/IR/Instructions.h>

namespace EmojicodeCompiler {

struct Capture {
    std::vector<VariableCapture> captures;
    /// The type of the captured type context. If this is TypeType::NoReturn, the type context is not captured.
    Type self = Type::noReturn();
    llvm::Type *type = nullptr;

    bool capturesSelf() const { return self.type() != TypeType::NoReturn; }
};

class ASTClosure : public ASTExpr, public MFHeapAutoAllocates {
public:
    ASTClosure(std::unique_ptr<Function> &&closure, const SourcePosition &p);

    Type analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const final;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) override;

private:
    std::unique_ptr<Function> closure_;
    Capture capture_;

    llvm::Value* storeCapturedVariables(FunctionCodeGenerator *fg, const Capture &capture) const;

    void applyBoxingFromExpectation(FunctionAnalyser *analyser, const TypeExpectation &expectation);
};

}  // namespace EmojicodeCompiler

#endif /* ASTClosure_hpp */
