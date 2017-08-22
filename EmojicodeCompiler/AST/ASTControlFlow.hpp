//
//  ASTControlFlow.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTControlFlow_hpp
#define ASTControlFlow_hpp

#include "ASTStatements.hpp"

namespace EmojicodeCompiler {

class ASTIf final : public ASTStatement {
public:
    using ASTStatement::ASTStatement;
    void addCondition(const std::shared_ptr<ASTExpr> &ptr) { conditions_.emplace_back(ptr); }
    void addBlock(const ASTBlock &ptr) { blocks_.emplace_back(ptr); }

    void analyse(SemanticAnalyser *) override;
    void generate(FnCodeGenerator *) const override;

    bool hasElse() const { return conditions_.size() < blocks_.size(); }
private:
    std::vector<std::shared_ptr<ASTExpr>> conditions_;
    std::vector<ASTBlock> blocks_;
};

class ASTRepeatWhile final : public ASTStatement {
public:
    ASTRepeatWhile(const std::shared_ptr<ASTExpr> &condition, const ASTBlock &block, const SourcePosition &p)
    : ASTStatement(p), condition_(condition), block_(block) {}

    void analyse(SemanticAnalyser *) override;
    void generate(FnCodeGenerator *) const override;
private:
    std::shared_ptr<ASTExpr> condition_;
    ASTBlock block_;
};

class ASTForIn final : public ASTStatement {
public:
    ASTForIn(const std::shared_ptr<ASTExpr> &iteratee, const EmojicodeString &varName, const ASTBlock &block,
             const SourcePosition &p)
    : ASTStatement(p), iteratee_(iteratee), block_(block), varName_(varName) {}

    void analyse(SemanticAnalyser *) override;
    void generate(FnCodeGenerator *) const override;
private:
    std::shared_ptr<ASTExpr> iteratee_;
    ASTBlock block_;
    VariableID elementVar_;
    VariableID iteratorVar_;
    EmojicodeString varName_;
    Type elementType_ = Type::nothingness();
};

class ASTErrorHandler final : public ASTStatement {
public:
    ASTErrorHandler(const std::shared_ptr<ASTExpr> &value, const EmojicodeString &varNameValue,
                    const EmojicodeString &varNameError, const ASTBlock &valueBlock, const ASTBlock &errorBlock,
                    const SourcePosition &p)
    : ASTStatement(p), value_(value), valueBlock_(valueBlock), errorBlock_(errorBlock),
    valueVarName_(varNameValue), errorVarName_(varNameError) {}

    void analyse(SemanticAnalyser *) override;
    void generate(FnCodeGenerator *) const override;
private:
    std::shared_ptr<ASTExpr> value_;
    bool valueIsBoxed_;
    VariableID varId_;
    ASTBlock valueBlock_;
    Type valueType_ = Type::nothingness();
    ASTBlock errorBlock_;
    EmojicodeString valueVarName_;
    EmojicodeString errorVarName_;
};

}

#endif /* ASTControlFlow_hpp */
