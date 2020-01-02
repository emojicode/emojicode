//
//  ASTClosure.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 16/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTClosure_hpp
#define ASTClosure_hpp

#include <utility>
#include "ASTExpr.hpp"
#include "ASTBoxing.hpp"
#include "Scoping/CapturingSemanticScoper.hpp"
#include "MemoryFlowAnalysis/MFHeapAllocates.hpp"

namespace llvm {
class Function;
}  // namespace llvm

namespace EmojicodeCompiler {

class CodeGenerator;

struct Capture {
    std::vector<VariableCapture> captures;
    /// The type of the captured type context. If this is TypeType::NoReturn, the type context is not captured.
    Type self = Type::noReturn();
    llvm::Type *type = nullptr;

    bool capturesSelf() const { return self.type() != TypeType::NoReturn; }
};

class ASTClosure : public ASTExpr, public MFHeapAutoAllocates {
public:
    ASTClosure(std::unique_ptr<Function> &&closure, const SourcePosition &p, bool isEscaping);

    Type analyse(ExpressionAnalyser *analyser) override;
    Type comply(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const final;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) override;

    ~ASTClosure() override;

private:
    std::unique_ptr<Function> closure_;
    Capture capture_;
    bool isEscaping_;

    llvm::Value* storeCapturedVariables(FunctionCodeGenerator *fg, const Capture &capture) const;

    void applyBoxingFromExpectation(ExpressionAnalyser *analyser, const TypeExpectation &expectation);
    llvm::Value* createDeinit(CodeGenerator *cg, const Capture &capture) const;
};

class ASTCallableBox final : public ASTBoxing, public MFHeapAutoAllocates {
public:
    ASTCallableBox(std::shared_ptr<ASTExpr> expr, const SourcePosition &p, const Type &exprType,
                   std::unique_ptr<Function> thunk);

    void analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) override;

    llvm::Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(PrettyStream &pretty) const override {}

    ~ASTCallableBox() override;

private:
    static llvm::Function* getRelease(CodeGenerator *cg);
    static llvm::Function *kRelease;
    std::unique_ptr<Function> thunk_;
};

class ASTCallableThunkDestination : public ASTExpr {
public:
    ASTCallableThunkDestination(const SourcePosition &p, Type destinationType)
        : ASTExpr(p), type_(std::move(destinationType)) {}

    Type analyse(ExpressionAnalyser *) final { return type_; }
    void toCode(PrettyStream &pretty) const override {}
    llvm::Value* generate(FunctionCodeGenerator *fg) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) override {}

private:
    Type type_;
};

}  // namespace EmojicodeCompiler

#endif /* ASTClosure_hpp */
