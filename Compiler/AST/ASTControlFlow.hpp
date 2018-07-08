//
//  ASTControlFlow.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTControlFlow_hpp
#define ASTControlFlow_hpp

#include <utility>

#include "ASTStatements.hpp"

namespace EmojicodeCompiler {

class ASTIf final : public ASTStatement {
public:
    using ASTStatement::ASTStatement;
    void addCondition(const std::shared_ptr<ASTExpr> &ptr) { conditions_.emplace_back(ptr); }
    void addBlock(ASTBlock ptr) { blocks_.emplace_back(std::move(ptr)); }

    void analyse(FunctionAnalyser *) override;
    void generate(FunctionCodeGenerator *) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser) override;

    bool hasElse() const { return conditions_.size() < blocks_.size(); }
private:
    std::vector<std::shared_ptr<ASTExpr>> conditions_;
    std::vector<ASTBlock> blocks_;
};

class ASTRepeatWhile final : public ASTStatement {
public:
    ASTRepeatWhile(std::shared_ptr<ASTExpr> condition, ASTBlock block, const SourcePosition &p)
    : ASTStatement(p), condition_(std::move(condition)), block_(std::move(block)) {}

    void analyse(FunctionAnalyser *) override;
    void generate(FunctionCodeGenerator *) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser) override;

protected:
    std::shared_ptr<ASTExpr> condition_;
    ASTBlock block_;
};

class ASTForIn final : public ASTStatement {
public:
    ASTForIn(std::shared_ptr<ASTExpr> iteratee, std::u32string varName, ASTBlock block,
             const SourcePosition &p)
    : ASTStatement(p), iteratee_(std::move(iteratee)), block_(std::move(block)), varName_(std::move(varName)) {}

    void analyse(FunctionAnalyser *) override;
    void generate(FunctionCodeGenerator *) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser) override;

private:
    std::shared_ptr<ASTExpr> iteratee_;
    ASTBlock block_;
    std::u32string varName_;
};

class ASTErrorHandler final : public ASTStatement {
public:
    ASTErrorHandler(std::shared_ptr<ASTExpr> value, std::u32string varNameValue,
                    std::u32string varNameError, ASTBlock valueBlock, ASTBlock errorBlock,
                    const SourcePosition &p)
    : ASTStatement(p), value_(std::move(value)), valueBlock_(std::move(valueBlock)), errorBlock_(std::move(errorBlock)),
    valueVarName_(std::move(varNameValue)), errorVarName_(std::move(varNameError)) {}

    void analyse(FunctionAnalyser *) override;
    void generate(FunctionCodeGenerator *) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser) override;
    
private:
    std::shared_ptr<ASTExpr> value_;
    bool valueIsBoxed_ = false;
    ASTBlock valueBlock_;
    Type valueType_ = Type::noReturn();
    ASTBlock errorBlock_;
    VariableID valueVar_;
    VariableID errorVar_;
    std::u32string valueVarName_;
    std::u32string errorVarName_;
};

}  // namespace EmojicodeCompiler

#endif /* ASTControlFlow_hpp */
