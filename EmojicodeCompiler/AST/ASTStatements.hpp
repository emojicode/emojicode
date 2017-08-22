//
//  ASTStatements.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTStatements_hpp
#define ASTStatements_hpp

#include "ASTNode.hpp"
#include "ASTExpr.hpp"
#include "../Types/TypeExpectation.hpp"
#include "../Scoping/CGScoper.hpp"

namespace EmojicodeCompiler {

class SemanticAnalyser;
class FnCodeGenerator;

class ASTStatement : public ASTNode {
public:
    virtual void generate(FnCodeGenerator *) const = 0;
    virtual void analyse(SemanticAnalyser *) = 0;
protected:
    using ASTNode::ASTNode;
};

class ASTBlock final : public ASTStatement {
public:
    ASTBlock(const SourcePosition &p) : ASTStatement(p) {}

    void appendNode(const std::shared_ptr<ASTStatement> &node) {
        stmts_.emplace_back(node);
    }

    void preprendNode(const std::shared_ptr<ASTStatement> &node) {
        stmts_.emplace(stmts_.begin(), node);
    }

    void analyse(SemanticAnalyser *analyser) override;
    void generate(FnCodeGenerator *) const override;

    const std::vector<std::shared_ptr<ASTStatement>>& nodes() const { return stmts_; }
private:
    std::vector<std::shared_ptr<ASTStatement>> stmts_;
};

class ASTExprStatement final : public ASTStatement {
public:
    void analyse(SemanticAnalyser *analyser) override;

    void generate(FnCodeGenerator *fncg) const override {
        expr_->generate(fncg);
    }

    ASTExprStatement(const std::shared_ptr<ASTExpr> &expr, const SourcePosition &p) : ASTStatement(p), expr_(expr) {}
private:
    std::shared_ptr<ASTExpr> expr_;
};

class ASTReturn : public ASTStatement {
public:
    ASTReturn(const std::shared_ptr<ASTExpr> &value, const SourcePosition &p) : ASTStatement(p), value_(value) {}

    void analyse(SemanticAnalyser *analyser) override;
    void generate(FnCodeGenerator *) const override;
protected:
    std::shared_ptr<ASTExpr> value_;
};

class ASTRaise final : public ASTReturn {
public:
    using ASTReturn::ASTReturn;

    void analyse(SemanticAnalyser *analyser) override;
    void generate(FnCodeGenerator *) const override;
private:
    bool boxed_ = false;
};

class ASTSuperinitializer final : public ASTStatement {
public:
    ASTSuperinitializer(const EmojicodeString &name, const ASTArguments &arguments,
                        const SourcePosition &p) : ASTStatement(p), name_(name), arguments_(arguments) {}

    void analyse(SemanticAnalyser *analyser) override;
    void generate(FnCodeGenerator *) const override;
private:
    EmojicodeString name_;
    ASTArguments arguments_;
    Type superType_ = Type::nothingness();
};
    
}

#endif /* ASTStatements_hpp */
