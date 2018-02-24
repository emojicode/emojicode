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

/// Represents the declaration of a variable.
class ASTVariableDeclaration final : public ASTStatement {
public:
    ASTVariableDeclaration(Type type, std::u32string name, const SourcePosition &p)
            : ASTStatement(p), varName_(std::move(name)), type_(std::move(type)) {}

    void analyse(SemanticAnalyser *analyser) override;
    void generate(FunctionCodeGenerator *) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    std::u32string varName_;
    Type type_;
    VariableID id_;
};

class AccessesAnyVariable {
public:
    /// @returns The name of the variable to be accessed.
    const std::u32string& name() const { return name_; }
protected:
    explicit AccessesAnyVariable(std::u32string name) : name_(std::move(name)) {}
    void setVariableAccess(const ResolvedVariable &var, SemanticAnalyser *analyser);
    bool inInstanceScope() const { return inInstanceScope_; }
    VariableID id() const { return id_; }

    /// Generates code to retrieve a pointer to the instance variable, whose ID is stored in this instance.
    Value* instanceVariablePointer(FunctionCodeGenerator *fg) const;
private:
    bool inInstanceScope_ = false;
    VariableID id_;
    std::u32string name_;
};

/// Represents the retrieval of the contents of a variable or the address of the content of the variable.
class ASTGetVariable final : public ASTExpr, public AccessesAnyVariable {
public:
    ASTGetVariable(std::u32string name, const SourcePosition &p) : ASTExpr(p), AccessesAnyVariable(std::move(name)) {}

    /// Configures this node to generate code to retrieve the variable’s address instead of its value.
    void setReference() { reference_ = true; }

    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    bool reference_ = false;
};

/// Every AST node that potentially initializes a variable, i.e. initializes a value type to its address, inherits
/// from this class. The act of initializing a variable may occur repeatedly per variable.
class ASTVariableInit : public ASTStatement, public AccessesAnyVariable {
protected:
    ASTVariableInit(std::shared_ptr<ASTExpr> e, const SourcePosition &p, std::u32string name)
            : ASTStatement(p), AccessesAnyVariable(std::move(name)), expr_(std::move(e)) {}
    std::shared_ptr<ASTExpr> expr_;

    /// Generates code to retrieve the pointer to the variable’s address, to which a value can be stored.
    /// If setDeclares() was used to configure this node, stack memory will be allocated.
    Value *variablePointer(FunctionCodeGenerator *fg) const;
    /// Configures this node to allocate stack memory for this variable before trying to initialize it.
    void setDeclares() { declare_ = true; }

    virtual void generateAssignment(FunctionCodeGenerator *) const = 0;
private:
    void generate(FunctionCodeGenerator *) const final;
    bool declare_ = false;
};

class ASTVariableAssignmentDecl : public ASTVariableInit {
public:
    ASTVariableAssignmentDecl(std::u32string name, const std::shared_ptr<ASTExpr> &e,
                              const SourcePosition &p) : ASTVariableInit(e, p, std::move(name)) {}
    void analyse(SemanticAnalyser *analyser) override;
    void generateAssignment(FunctionCodeGenerator *) const final;
    void toCode(Prettyprinter &pretty) const override;
};

/// Inserted by the compiler to initialize an instance variable as specified by a baby bottle initializer.
class ASTInstanceVariableInitialization final : public ASTVariableAssignmentDecl {
public:
    using ASTVariableAssignmentDecl::ASTVariableAssignmentDecl;
    void analyse(SemanticAnalyser *analyser) override;
    void toCode(Prettyprinter &pretty) const override {}
};

class ASTFrozenDeclaration final : public ASTVariableInit {
public:
    ASTFrozenDeclaration(std::u32string name, const std::shared_ptr<ASTExpr> &e,
                         const SourcePosition &p) : ASTVariableInit(e, p, std::move(name)) {}

    void analyse(SemanticAnalyser *analyser) override;
    void generateAssignment(FunctionCodeGenerator *) const override;
    void toCode(Prettyprinter &pretty) const override;
};

}  // namespace EmojicodeCompiler

#endif /* ASTVariables_hpp */
