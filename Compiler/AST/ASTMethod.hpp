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
        Equal, Store, Load, IsNoValueLeft, IsNoValueRight,
    };

    BuiltInType builtIn_ = BuiltInType::None;
    ASTArguments args_;
    CallType callType_ = CallType::None;
    Type calleeType_ = Type::noReturn();
private:
    bool builtIn(FunctionAnalyser *analyser, const Type &type, const std::u32string &name);

    Type analyseMultiProtocolCall(FunctionAnalyser *analyser, const std::u32string &name, const Type &type);

    void checkMutation(FunctionAnalyser *analyser, const std::shared_ptr<ASTExpr> &callee, const Type &type,
                       const Function *method) const;
    void determineCallType(const FunctionAnalyser *analyser, const Type &type);
};

class ASTMethod final : public ASTMethodable {
public:
    ASTMethod(std::u32string name, std::shared_ptr<ASTExpr> callee, const ASTArguments &args, const SourcePosition &p)
    : ASTMethodable(p, args), name_(std::move(name)), callee_(std::move(callee)) {}
    Type analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) override;
    void toCode(Prettyprinter &pretty) const override;
    Value* generate(FunctionCodeGenerator *fg) const override;
private:
    std::u32string name_;
    std::shared_ptr<ASTExpr> callee_;
};

}  // namespace EmojicodeCompiler

#endif /* ASTMethod_hpp */
