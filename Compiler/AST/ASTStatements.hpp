//
//  ASTStatements.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTStatements_hpp
#define ASTStatements_hpp

#include <utility>

#include "ASTExpr.hpp"
#include "ASTNode.hpp"
#include "Scoping/CGScoper.hpp"
#include "Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

class FunctionAnalyser;
class FunctionCodeGenerator;

class ASTStatement : public ASTNode {
public:
    virtual void generate(FunctionCodeGenerator *) const = 0;
    virtual void analyse(FunctionAnalyser *) = 0;
    virtual void toCode(Prettyprinter &pretty) const = 0;
    void setParagraph() { paragraph_ = true; }
    bool paragraph() const { return paragraph_; }
protected:
    using ASTNode::ASTNode;
    bool paragraph_ = false;
};

class ASTBlock final : public ASTStatement {
public:
    explicit ASTBlock(const SourcePosition &p) : ASTStatement(p) {}

    void appendNode(const std::shared_ptr<ASTStatement> &node) {
        assert(!returnedCertainly_);
        stmts_.emplace_back(node);
    }

    void preprendNode(const std::shared_ptr<ASTStatement> &node) {
        stmts_.emplace(stmts_.begin(), node);
    }

    void analyse(FunctionAnalyser *analyser) override;
    void generate(FunctionCodeGenerator *) const override;
    void toCode(Prettyprinter &pretty) const override;
    /// Prints the code that goes between the block delimiters.
    void innerToCode(Prettyprinter &pretty) const;
    bool returnedCertainly() const { return returnedCertainly_; }

    const std::vector<std::shared_ptr<ASTStatement>>& nodes() const { return stmts_; }
private:
    std::vector<std::shared_ptr<ASTStatement>> stmts_;
    bool returnedCertainly_ = false;
    size_t stop_ = 0;
};

class ASTExprStatement final : public ASTStatement {
public:
    void analyse(FunctionAnalyser *analyser) override;

    void generate(FunctionCodeGenerator *fg) const override {
        expr_->generate(fg);
    }
    void toCode(Prettyprinter &pretty) const override;

    ASTExprStatement(std::shared_ptr<ASTExpr> expr, const SourcePosition &p) : ASTStatement(p), expr_(std::move(expr)) {}
private:
    std::shared_ptr<ASTExpr> expr_;
};

class ASTReturn : public ASTStatement {
public:
    ASTReturn(std::shared_ptr<ASTExpr> value, const SourcePosition &p) : ASTStatement(p), value_(std::move(value)) {}

    void analyse(FunctionAnalyser *analyser) override;
    void generate(FunctionCodeGenerator *) const override;
    void toCode(Prettyprinter &pretty) const override;
protected:
    std::shared_ptr<ASTExpr> value_;
};

class ASTRaise final : public ASTReturn {
public:
    using ASTReturn::ASTReturn;

    void analyse(FunctionAnalyser *analyser) override;
    void generate(FunctionCodeGenerator *) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    bool boxed_ = false;
};

} // namespace EmojicodeCompiler

#endif /* ASTStatements_hpp */
