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

class ASTGetVariable : public ASTExpr, public ASTVariable {
public:
    ASTGetVariable(std::u32string name, const SourcePosition &p) : ASTExpr(p), name_(std::move(name)) {}

    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generateExpr(FnCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override;

    void setReference() { reference_ = true; }
    bool reference() { return reference_; }
    const std::u32string& name() { return name_; }

    static Value* instanceVariablePointer(FnCodeGenerator *fncg, size_t index);
private:
    bool reference_ = false;
    std::u32string name_;
};

class ASTInitGetVariable final : public ASTGetVariable {
public:
    ASTInitGetVariable(VariableID varId, bool inInstanceScope, bool declare, Type type, const SourcePosition &p) :
    ASTGetVariable(std::u32string(), p), declare_(declare), type_(std::move(type)) {
        inInstanceScope_ = inInstanceScope;
        varId_ = varId;
        setReference();
    }
    Value* generateExpr(FnCodeGenerator *fncg) const override;
private:
    bool declare_ = false;
    Type type_;
};


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
    void toCode(Prettyprinter &pretty) const override;
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
    void toCode(Prettyprinter &pretty) const override;
protected:
    std::u32string varName_;
private:
    bool declare_ = false;
};

class ASTInstanceVariableAssignment final : public ASTVariableAssignmentDecl {
public:
    using ASTVariableAssignmentDecl::ASTVariableAssignmentDecl;
    void analyse(SemanticAnalyser *analyser) override;
    void toCode(Prettyprinter &pretty) const override {}
};

class ASTFrozenDeclaration final : public ASTInitableCreator {
public:
    ASTFrozenDeclaration(std::u32string name, const std::shared_ptr<ASTExpr> &e,
                         const SourcePosition &p) : ASTInitableCreator(e, p), varName_(std::move(name)) {}

    void analyse(SemanticAnalyser *analyser) override;
    void generateAssignment(FnCodeGenerator *) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    std::u32string varName_;
    VariableID id_;
};

}  // namespace EmojicodeCompiler

#endif /* ASTVariables_hpp */
