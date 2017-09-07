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

#include "../Scoping/CGScoper.hpp"
#include "../Types/TypeExpectation.hpp"
#include "ASTExpr.hpp"
#include "ASTNode.hpp"

namespace EmojicodeCompiler {

class SemanticAnalyser;
class FunctionCodeGenerator;

class ASTStatement : public ASTNode {
public:
    virtual void generate(FunctionCodeGenerator *) const = 0;
    virtual void analyse(SemanticAnalyser *) = 0;
    virtual void toCode(Prettyprinter &pretty) const = 0;
protected:
    using ASTNode::ASTNode;
};

class ASTBlock final : public ASTStatement {
public:
    explicit ASTBlock(const SourcePosition &p) : ASTStatement(p) {}

    void appendNode(const std::shared_ptr<ASTStatement> &node) {
        stmts_.emplace_back(node);
    }

    void preprendNode(const std::shared_ptr<ASTStatement> &node) {
        stmts_.emplace(stmts_.begin(), node);
    }

    void analyse(SemanticAnalyser *analyser) override;
    void generate(FunctionCodeGenerator *) const override;
    void toCode(Prettyprinter &pretty) const override;
    /// Prints the code that goes between the block delimiters.
    void innerToCode(Prettyprinter &pretty) const;

    const std::vector<std::shared_ptr<ASTStatement>>& nodes() const { return stmts_; }
private:
    std::vector<std::shared_ptr<ASTStatement>> stmts_;
};

class ASTExprStatement final : public ASTStatement {
public:
    void analyse(SemanticAnalyser *analyser) override;

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

    void analyse(SemanticAnalyser *analyser) override;
    void generate(FunctionCodeGenerator *) const override;
    void toCode(Prettyprinter &pretty) const override;
protected:
    std::shared_ptr<ASTExpr> value_;
};

class ASTRaise final : public ASTReturn {
public:
    using ASTReturn::ASTReturn;

    void analyse(SemanticAnalyser *analyser) override;
    void generate(FunctionCodeGenerator *) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    bool boxed_ = false;
};

class ASTSuperinitializer final : public ASTStatement {
public:
    ASTSuperinitializer(std::u32string name, ASTArguments arguments,
                        const SourcePosition &p) : ASTStatement(p), name_(std::move(name)), arguments_(std::move(arguments)) {}

    void analyse(SemanticAnalyser *analyser) override;
    void generate(FunctionCodeGenerator *) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    std::u32string name_;
    ASTArguments arguments_;
    Type superType_ = Type::noReturn();
};
    
} // namespace EmojicodeCompiler

#endif /* ASTStatements_hpp */
