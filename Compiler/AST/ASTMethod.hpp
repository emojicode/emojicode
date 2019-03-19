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
#include "Functions/CallType.h"
#include <utility>

namespace EmojicodeCompiler {

class FunctionAnalyser;

class ASTMethodable : public ASTCall {
protected:
    explicit ASTMethodable(const SourcePosition &p) : ASTCall(p), args_(p) {}
    ASTMethodable(const SourcePosition &p, ASTArguments args) : ASTCall(p), args_(std::move(args)) {}

    Type analyseMethodCall(ExpressionAnalyser *analyser, const std::u32string &name,
                           std::shared_ptr<ASTExpr> &callee);
    /// Analyses this node as method call.
    /// @param otype Result of analysing `callee` with TypeExpectation `TypeExpectation()`.
    Type analyseMethodCall(ExpressionAnalyser *analyser, const std::u32string &name, std::shared_ptr<ASTExpr> &callee,
                           const Type &otype);

    enum class BuiltInType {
        None,
        DoubleMultiply, DoubleAdd, DoubleSubstract, DoubleDivide, DoubleGreater, DoubleGreaterOrEqual,
        DoubleLess, DoubleLessOrEqual, DoubleRemainder, DoubleEqual, DoubleInverse,
        IntegerMultiply, IntegerAdd, IntegerSubstract, IntegerDivide, IntegerGreater, IntegerGreaterOrEqual,
        IntegerLess, IntegerLessOrEqual, IntegerLeftShift, IntegerRightShift, IntegerOr, IntegerAnd, IntegerXor,
        IntegerRemainder, IntegerToDouble, IntegerNot, IntegerInverse, IntegerToByte, ByteToInteger,
        BooleanAnd, BooleanOr, BooleanNegate,
        Equal, Store, Load, Release, MemoryMove, MemorySet, IsNoValueLeft, IsNoValueRight, Multiprotocol,
    };

    BuiltInType builtIn_ = BuiltInType::None;
    ASTArguments args_;
    CallType callType_ = CallType::None;
    Type calleeType_ = Type::noReturn();
    size_t multiprotocolN_ = 0;
    Function *method_ = nullptr;

    bool isErrorProne() const override;
    const Type& errorType() const override;

private:
    bool builtIn(ExpressionAnalyser *analyser, const Type &type, const std::u32string &name);

    Type analyseMultiProtocolCall(ExpressionAnalyser *analyser, const std::u32string &name);

    void checkMutation(ExpressionAnalyser *analyser, const std::shared_ptr<ASTExpr> &callee) const;
    void determineCallType(const ExpressionAnalyser *analyser);
    void determineCalleeType(ExpressionAnalyser *analyser, const std::u32string &name,
                             std::shared_ptr<ASTExpr> &callee, const Type &otype);
    Type analyseTypeMethodCall(ExpressionAnalyser *analyser, const std::u32string &name,
                               std::shared_ptr<ASTExpr> &callee);
};

class ASTMethod final : public ASTMethodable {
public:
    ASTMethod(std::u32string name, std::shared_ptr<ASTExpr> callee, const ASTArguments &args, const SourcePosition &p)
    : ASTMethodable(p, args), name_(std::move(name)), callee_(std::move(callee)) {}
    Type analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;
    void toCode(PrettyStream &pretty) const override;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) override;
    void mutateReference(ExpressionAnalyser *analyser) final;

private:
    std::u32string name_;
    std::shared_ptr<ASTExpr> callee_;

    llvm::Value* buildMemoryAddress(FunctionCodeGenerator *fg, llvm::Value *memory, llvm::Value *offset,
                                    const Type &type) const;
    llvm::Value* buildAddOffsetAddress(FunctionCodeGenerator *fg, llvm::Value *memory, llvm::Value *offset) const;
};
    
}  // namespace EmojicodeCompiler

#endif /* ASTMethod_hpp */
