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
#include "Scoping/SemanticScopeStats.hpp"
#include "ASTExpr.hpp"
#include "Scoping/IDScoper.hpp"
#include "ErrorSelfDestructing.hpp"
#include "Releasing.hpp"

namespace EmojicodeCompiler {

class FunctionAnalyser;
class FunctionCodeGenerator;
class ASTReturn;

class ASTStatement : public ASTNode {
public:
    virtual void generate(FunctionCodeGenerator *) const = 0;
    virtual void analyse(FunctionAnalyser *) = 0;
    virtual void analyseMemoryFlow(MFFunctionAnalyser *) = 0;

    void setParagraph() { paragraph_ = true; }
    bool paragraph() const { return paragraph_; }
protected:
    using ASTNode::ASTNode;
    bool paragraph_ = false;
};

class ASTBlock final : public ASTStatement {
public:
    explicit ASTBlock(const SourcePosition &p) : ASTStatement(p) {}

    /// Appends a statement to the block.
    /// @warning If you append a return statement after semantic analysis call setReturnedCertainly().
    void appendNode(std::unique_ptr<ASTStatement> node) { stmts_.emplace_back(std::move(node)); }

    void prependNode(std::unique_ptr<ASTStatement> node) { stmts_.emplace(stmts_.begin(), std::move(node)); }

    void analyse(FunctionAnalyser *analyser) override;
    void generate(FunctionCodeGenerator *) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser) override;

    void toCode(PrettyStream &pretty) const override;
    /// Prints the code that goes between the block delimiters.
    void innerToCode(PrettyStream &pretty) const;

    /// Returns true iff in this block (or a subblock) a return or raise statement will certainly be executed.
    bool returnedCertainly() const { return returnedCertainly_; }
    /// Makes returnedCertainly() return true.
    void setReturnedCertainly() { returnedCertainly_ = true; stop_ = stmts_.size(); }

    const SemanticScopeStats& scopeStats() { assert(hasStats_); return scopeStats_; }

    void popScope(FunctionAnalyser *analyser);

    /// Returns the last statement in the block that is compiled (i.e. that is not dead code) if it is a return
    /// or raise statement.
    /// @note This method can only be called before Memory Flow Analysis.
    ASTReturn* getReturn() const;

    void setBeginIndex(size_t index) { beginIndex_ = index; }
    void setEndIndex(size_t index) { endIndex_ = index; }

    size_t beginIndex() const { return beginIndex_; }
    size_t endIndex() const { return endIndex_; }

    size_t stmtsSize() const { return stmts_.size(); }
    
private:
    std::vector<std::unique_ptr<ASTStatement>> stmts_;
    bool returnedCertainly_ = false;
    size_t stop_ = 0;
    size_t beginIndex_ = 0;
    size_t endIndex_ = 0;
    bool hasStats_ = false;
    SemanticScopeStats scopeStats_;
};

class ASTExprStatement final : public ASTStatement {
public:
    void analyse(FunctionAnalyser *analyser) override;

    void generate(FunctionCodeGenerator *fg) const override {
        expr_->generate(fg);
    }

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser) override;

    ASTExprStatement(std::shared_ptr<ASTExpr> expr, const SourcePosition &p) : ASTStatement(p), expr_(std::move(expr)) {}
private:
    std::shared_ptr<ASTExpr> expr_;
};

class ASTReturn : public ASTStatement, public Releasing {
public:
    ASTReturn(std::shared_ptr<ASTExpr> value, const SourcePosition &p);

    void analyse(FunctionAnalyser *analyser) override;
    void generate(FunctionCodeGenerator *) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser) override;

    /// Informs the expression that it is used to return the initialized object from an object initializer.
    void setIsInitReturn() { initReturn_ = true; }

protected:
    void returnReference(FunctionAnalyser *analyser, Type type);

    std::shared_ptr<ASTExpr> value_;
    bool initReturn_ = false;
};

class ASTRaise final : public ASTReturn, private ErrorSelfDestructing {
public:
    using ASTReturn::ASTReturn;

    void analyse(FunctionAnalyser *analyser) override;
    void generate(FunctionCodeGenerator *) const override;
    void toCode(PrettyStream &pretty) const override;
};

} // namespace EmojicodeCompiler

#endif /* ASTStatements_hpp */
