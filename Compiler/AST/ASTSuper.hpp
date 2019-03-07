//
//  ASTSuper.hpp
//  runtime
//
//  Created by Theo Weidmann on 07.03.19.
//

#ifndef ASTSuper_hpp
#define ASTSuper_hpp

#include "ErrorSelfDestructing.hpp"
#include "ASTExpr.hpp"

namespace EmojicodeCompiler {

class ASTSuper final : public ASTCall, private ErrorSelfDestructing, private ErrorHandling {
public:
    ASTSuper(std::u32string name, ASTArguments args, const SourcePosition &p)
    : ASTCall(p), name_(std::move(name)), args_(std::move(args)) {}
    Type analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override;

    const Type& errorType() const override;
    bool isErrorProne() const override;

private:
    void analyseSuperInit(ExpressionAnalyser *analyser);
    std::u32string name_;
    Function *function_ = nullptr;
    Type calleeType_ = Type::noReturn();
    ASTArguments args_;
    bool init_ = false;
    bool manageErrorProneness_ = false;

    void analyseSuperInitErrorProneness(ExpressionAnalyser *analyser, const Initializer *initializer);
};

}  // namespace EmojicodeCompiler

#endif /* ASTSuper_hpp */
