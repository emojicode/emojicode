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

class ASTMethodable : public ASTExpr {
protected:
    explicit ASTMethodable(const SourcePosition &p) : ASTExpr(p), args_(p) {}
    ASTMethodable(const SourcePosition &p, ASTArguments args) : ASTExpr(p), args_(std::move(args)) {}

    Type analyseMethodCall(FunctionAnalyser *analyser, const std::u32string &name,
                           std::shared_ptr<ASTExpr> &callee);
    /// Analyses this node as method call.
    /// @param otype Result of analysing `callee` with TypeExpectation `TypeExpectation()`.
    Type analyseMethodCall(FunctionAnalyser *analyser, const std::u32string &name, std::shared_ptr<ASTExpr> &callee,
                           const Type &otype);

    enum class BuiltInType {
        None,
        DoubleMultiply, DoubleAdd, DoubleSubstract, DoubleDivide, DoubleGreater, DoubleGreaterOrEqual,
        DoubleLess, DoubleLessOrEqual, DoubleRemainder, DoubleEqual,
        IntegerMultiply, IntegerAdd, IntegerSubstract, IntegerDivide, IntegerGreater, IntegerGreaterOrEqual,
        IntegerLess, IntegerLessOrEqual, IntegerLeftShift, IntegerRightShift, IntegerOr, IntegerAnd, IntegerXor,
        IntegerRemainder, IntegerToDouble, IntegerNot,
        BooleanAnd, BooleanOr, BooleanNegate,
        Equal, Store, Load, Release, MemoryMove, MemorySet, IsNoValueLeft, IsNoValueRight, Multiprotocol,
    };

    BuiltInType builtIn_ = BuiltInType::None;
    ASTArguments args_;
    CallType callType_ = CallType::None;
    Type calleeType_ = Type::noReturn();
    size_t multiprotocolN_ = 0;
    Function *method_ = nullptr;
private:
    bool builtIn(FunctionAnalyser *analyser, const Type &type, const std::u32string &name);

    Type analyseMultiProtocolCall(FunctionAnalyser *analyser, const std::u32string &name);

    void checkMutation(FunctionAnalyser *analyser, const std::shared_ptr<ASTExpr> &callee, const Type &type) const;
    void determineCallType(const FunctionAnalyser *analyser);
    void determineCalleeType(FunctionAnalyser *analyser, const std::u32string &name,
                             std::shared_ptr<ASTExpr> &callee, const Type &otype);
    Type analyseTypeMethodCall(FunctionAnalyser *analyser, const std::u32string &name,
                               std::shared_ptr<ASTExpr> &callee);
};

class ASTMethod final : public ASTMethodable {
public:
    ASTMethod(std::u32string name, std::shared_ptr<ASTExpr> callee, const ASTArguments &args, const SourcePosition &p)
    : ASTMethodable(p, args), name_(std::move(name)), callee_(std::move(callee)) {}
    Type analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) override;
    void toCode(PrettyStream &pretty) const override;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser, MFType type) override;

private:
    std::u32string name_;
    std::shared_ptr<ASTExpr> callee_;

    llvm::Value* buildMemoryAddress(FunctionCodeGenerator *fg, llvm::Value *memory, llvm::Value *offset,
                                    const Type &type) const;
    llvm::Value* buildAddOffsetAddress(FunctionCodeGenerator *fg, llvm::Value *memory, llvm::Value *offset) const;
};

}  // namespace EmojicodeCompiler

#endif /* ASTMethod_hpp */
