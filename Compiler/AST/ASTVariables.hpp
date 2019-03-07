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
#include <utility>
#include "Scoping/Variable.hpp"

namespace EmojicodeCompiler {

struct ResolvedVariable;

/// Represents the declaration of a variable.
class ASTVariableDeclaration final : public ASTStatement {
public:
    ASTVariableDeclaration(std::unique_ptr<ASTType> type, std::u32string name, const SourcePosition &p);

    void analyse(FunctionAnalyser *analyser) override;
    void generate(FunctionCodeGenerator *) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *) override {}

    ~ASTVariableDeclaration();

private:
    std::u32string varName_;
    std::unique_ptr<ASTType> type_;
    VariableID id_;
};

class AccessesAnyVariable {
public:
    /// @returns The name of the variable to be accessed.
    const std::u32string& name() const { return name_; }
    VariableID id() const { return id_; }
    bool inInstanceScope() const { return inInstanceScope_; }
protected:
    explicit AccessesAnyVariable(std::u32string name) : name_(std::move(name)) {}
    AccessesAnyVariable(bool inInstanceScope, VariableID id, Type type)
        : inInstanceScope_(inInstanceScope), id_(id), variableType_(std::move(type)) {}
    void setVariableAccess(const ResolvedVariable &var, ExpressionAnalyser *analyser);

    /// Generates code to retrieve a pointer to the instance variable, whose ID is stored in this instance.
    Value* instanceVariablePointer(FunctionCodeGenerator *fg) const;

    const Type& variableType() const { return variableType_; }

    /// Releases the object pointed to by this variable.
    void release(FunctionCodeGenerator *fg) const;

    Value* managementValue(FunctionCodeGenerator *fg) const;

private:
    bool inInstanceScope_ = false;
    VariableID id_;
    std::u32string name_;
    Type variableType_ = Type::noReturn();
};

/// Represents the retrieval of the contents of a variable or the address of the content of the variable.
class ASTGetVariable final : public ASTExpr, public AccessesAnyVariable {
public:
    ASTGetVariable(std::u32string name, const SourcePosition &p) : ASTExpr(p), AccessesAnyVariable(std::move(name)) {}

    /// Configures this node to generate code to retrieve the variable’s address instead of its value.
    void setReference() { reference_ = true; }

    Type analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) override;

    void mutateReference(ExpressionAnalyser *analyser) override;

private:
    bool reference_ = false;
    /// Set to true if the value created by the expression is returned. If it is returned, the value of the variable
    /// must not be retained.
    bool returned_ = false;
};

class ASTIsOnlyReference final : public ASTExpr, public AccessesAnyVariable {
public:
    ASTIsOnlyReference(std::u32string name, const SourcePosition &p)
        : ASTExpr(p), AccessesAnyVariable(std::move(name)) {}

    Type analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) override;
};

/// Every AST node that potentially initializes a variable, i.e. initializes a value type to its address, inherits
/// from this class. The act of initializing a variable may occur repeatedly per variable.
class ASTVariableInit : public ASTStatement, public AccessesAnyVariable {
protected:
    ASTVariableInit(std::shared_ptr<ASTExpr> e, const SourcePosition &p, std::u32string name, bool declare)
            : ASTStatement(p), AccessesAnyVariable(std::move(name)), expr_(std::move(e)), declare_(declare) {}
    std::shared_ptr<ASTExpr> expr_;

    /// Generates code to retrieve the pointer to the variable’s address, to which a value can be stored.
    /// If setDeclares() was used to configure this node, stack memory will be allocated.
    Value *variablePointer(FunctionCodeGenerator *fg) const;

    virtual void generateAssignment(FunctionCodeGenerator *) const = 0;
private:
    void generate(FunctionCodeGenerator *) const final;
    const bool declare_;
};

class ASTVariableAssignment : public ASTVariableInit {
public:
    ASTVariableAssignment(std::u32string name, const std::shared_ptr<ASTExpr> &e,
                          const SourcePosition &p) : ASTVariableInit(e, p, std::move(name), false) {}
    void analyse(FunctionAnalyser *analyser) override;
    void generateAssignment(FunctionCodeGenerator *) const final;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser) override;

protected:
    ASTVariableAssignment(std::u32string name, const std::shared_ptr<ASTExpr> &e,
                          const SourcePosition &p, bool declare) : ASTVariableInit(e, p, std::move(name), declare) {}

private:
    bool wasInitialized_ = false;
};

class ASTVariableDeclareAndAssign : public ASTVariableAssignment {
public:
    ASTVariableDeclareAndAssign(std::u32string name, const std::shared_ptr<ASTExpr> &e,
                                const SourcePosition &p) : ASTVariableAssignment(std::move(name), e, p, true) {}
    void analyse(FunctionAnalyser *analyser) override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser) override;
};

/// Inserted by the compiler to initialize an instance variable as specified by a baby bottle initializer.
class ASTInstanceVariableInitialization final : public ASTVariableAssignment {
public:
    ASTInstanceVariableInitialization(std::u32string name, const std::shared_ptr<ASTExpr> &e,
                                      const SourcePosition &p, bool analyseExpr)
        : ASTVariableAssignment(name, e, p), analyseExpr_(analyseExpr) {}
    void analyse(FunctionAnalyser *analyser) override;
    void toCode(PrettyStream &pretty) const override {}

private:
    bool analyseExpr_;
};

class ASTConstantVariable final : public ASTVariableInit {
public:
    ASTConstantVariable(std::u32string name, const std::shared_ptr<ASTExpr> &e,
                         const SourcePosition &p) : ASTVariableInit(e, p, std::move(name), true) {}

    void analyse(FunctionAnalyser *analyser) override;
    void generateAssignment(FunctionCodeGenerator *) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *) override;
};

enum class OperatorType;

class ASTOperatorAssignment final : public ASTVariableAssignment {
public:
    ASTOperatorAssignment(std::u32string name, const std::shared_ptr<ASTExpr> &e, const SourcePosition &p,
                          OperatorType opType);
    void toCode(PrettyStream &pretty) const override;
};

}  // namespace EmojicodeCompiler

#endif /* ASTVariables_hpp */
