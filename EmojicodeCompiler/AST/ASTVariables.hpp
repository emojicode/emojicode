//
//  ASTVariables.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTVariables_hpp
#define ASTVariables_hpp

#include <utility>

#include "ASTStatements.hpp"

namespace EmojicodeCompiler {

class ASTInitableCreator : public ASTStatement {
protected:
    ASTInitableCreator(std::shared_ptr<ASTExpr> e, const SourcePosition &p) : ASTStatement(p), expr_(std::move(e)) {}
    std::shared_ptr<ASTExpr> expr_;

    void setVtDestination(VariableID varId, bool inInstanceScope, bool declare);
    virtual void generateAssignment(FnCodeGenerator *) const = 0;
private:
    void generate(FnCodeGenerator *) const final;
    bool noAction_ = false;
};

class ASTVariableDeclaration final : public ASTStatement {
public:
    ASTVariableDeclaration(Type type, EmojicodeString name, const SourcePosition &p)
    : ASTStatement(p), varName_(std::move(name)), type_(std::move(type)) {}

    void analyse(SemanticAnalyser *analyser) override;
    void generate(FnCodeGenerator *) const override;
private:
    EmojicodeString varName_;
    Type type_;
    VariableID id_;
};

class ASTVariableAssignmentDecl : public ASTVariable, public ASTInitableCreator {
public:
    ASTVariableAssignmentDecl(EmojicodeString name, const std::shared_ptr<ASTExpr> &e,
                              const SourcePosition &p) : ASTInitableCreator(e, p), varName_(std::move(name)) {}
    void analyse(SemanticAnalyser *analyser) override;
    void generateAssignment(FnCodeGenerator *) const override;
protected:
    EmojicodeString varName_;
private:
    bool declare_ = false;

    CGScoper::Variable& generateGetVariable(FnCodeGenerator *fncg) const;
};

class ASTInstanceVariableAssignment final : public ASTVariableAssignmentDecl {
public:
    using ASTVariableAssignmentDecl::ASTVariableAssignmentDecl;
    void analyse(SemanticAnalyser *analyser) override;
};

class ASTFrozenDeclaration final : public ASTInitableCreator {
public:
    ASTFrozenDeclaration(EmojicodeString name, const std::shared_ptr<ASTExpr> &e,
                         const SourcePosition &p) : ASTInitableCreator(e, p), varName_(std::move(name)) {}

    void analyse(SemanticAnalyser *analyser) override;
    void generateAssignment(FnCodeGenerator *) const override;
private:
    EmojicodeString varName_;
    VariableID id_;
};

}  // namespace EmojicodeCompiler

#endif /* ASTVariables_hpp */
