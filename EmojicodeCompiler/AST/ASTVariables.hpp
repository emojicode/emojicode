//
//  ASTVariables.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTVariables_hpp
#define ASTVariables_hpp

#include "ASTStatements.hpp"
#include <sstream>
#include <utility>

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
    ASTVariableDeclaration(Type type, std::u32string name, const SourcePosition &p)
    : ASTStatement(p), varName_(std::move(name)), type_(std::move(type)) {}

    void analyse(SemanticAnalyser *analyser) override;
    void generate(FnCodeGenerator *) const override;
    void toCode(std::stringstream &stream, unsigned int indentation) const override;
private:
    std::u32string varName_;
    Type type_;
    VariableID id_;
};

class ASTVariableAssignmentDecl : public ASTVariable, public ASTInitableCreator {
public:
    ASTVariableAssignmentDecl(std::u32string name, const std::shared_ptr<ASTExpr> &e,
                              const SourcePosition &p) : ASTInitableCreator(e, p), varName_(std::move(name)) {}
    void analyse(SemanticAnalyser *analyser) override;
    void generateAssignment(FnCodeGenerator *) const override;
    void toCode(std::stringstream &stream, unsigned int indentation) const override;
protected:
    std::u32string varName_;
private:
    bool declare_ = false;

    CGScoper::Variable& generateGetVariable(FnCodeGenerator *fncg) const;
};

class ASTInstanceVariableAssignment final : public ASTVariableAssignmentDecl {
public:
    using ASTVariableAssignmentDecl::ASTVariableAssignmentDecl;
    void analyse(SemanticAnalyser *analyser) override;
    void toCode(std::stringstream &stream, unsigned int indentation) const override {}
};

class ASTFrozenDeclaration final : public ASTInitableCreator {
public:
    ASTFrozenDeclaration(std::u32string name, const std::shared_ptr<ASTExpr> &e,
                         const SourcePosition &p) : ASTInitableCreator(e, p), varName_(std::move(name)) {}

    void analyse(SemanticAnalyser *analyser) override;
    void generateAssignment(FnCodeGenerator *) const override;
    void toCode(std::stringstream &stream, unsigned int indentation) const override;
private:
    std::u32string varName_;
    VariableID id_;
};

}  // namespace EmojicodeCompiler

#endif /* ASTVariables_hpp */
