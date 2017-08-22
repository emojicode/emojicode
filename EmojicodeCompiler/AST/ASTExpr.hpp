//
//  ASTExpr.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTExpr_hpp
#define ASTExpr_hpp

#include "ASTNode.hpp"
#include "../Parsing/OperatorHelper.hpp"

namespace EmojicodeCompiler {

class ASTTypeExpr;
class SemanticAnalyser;
class TypeExpectation;
class FnCodeGenerator;
class CGScoper;

class ASTExpr : public ASTNode {
public:
    ASTExpr(const SourcePosition &p) : ASTNode(p) {}
    /// Set after semantic analysis and transformation.
    /// Iff this node represents an expression type this type is the exact type produced by this node.
    const Type& expressionType() const { return expressionType_; }
    void setExpressionType(const Type &type) { expressionType_ = type; }
    void setTemporarilyScoped() { temporarilyScoped_ = true; }

    void generate(FnCodeGenerator *fncg) const;
    virtual Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) = 0;
protected:
    virtual void generateExpr(FnCodeGenerator *fncg) const = 0;
private:
    Type expressionType_ = Type::nothingness();
    bool temporarilyScoped_ = false;
};

class ASTGetVariable final : public ASTExpr, public ASTVariable {
public:
    ASTGetVariable(const EmojicodeString &name, const SourcePosition &p) : ASTExpr(p), name_(name) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;

    void setReference() { reference_ = true; }
    bool reference() { return reference_; }
    const EmojicodeString& name() { return name_; }
private:
    bool reference_ = false;
    EmojicodeString name_;
};

class ASTMetaTypeInstantiation final : public ASTExpr {
public:
    ASTMetaTypeInstantiation(const Type &type, const SourcePosition &p) : ASTExpr(p), type_(type) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
private:
    Type type_;
};

class ASTArguments final : public ASTNode {
public:
    ASTArguments(const SourcePosition &p) : ASTNode(p) {}
    void addGenericArgument(const Type &type) { genericArguments_.emplace_back(type); }
    std::vector<Type>& genericArguments() { return genericArguments_; }
    void addArguments(const std::shared_ptr<ASTExpr> &arg) { arguments_.emplace_back(arg); }
    std::vector<std::shared_ptr<ASTExpr>>& arguments() { return arguments_; }
    const std::vector<std::shared_ptr<ASTExpr>>& arguments() const { return arguments_; }
private:
    std::vector<Type> genericArguments_;
    std::vector<std::shared_ptr<ASTExpr>> arguments_;
};

class ASTCast final : public ASTExpr {
public:
    ASTCast(const std::shared_ptr<ASTExpr> &value, const std::shared_ptr<ASTTypeExpr> &type,
            const SourcePosition &p) : ASTExpr(p), value_(value), typeExpr_(type) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
private:
    enum class CastType {
        ClassDowncast, ToClass, ToProtocol, ToValueType,
    };
    CastType castType_;
    std::shared_ptr<ASTExpr> value_;
    std::shared_ptr<ASTTypeExpr> typeExpr_;
};

class ASTCallableCall final : public ASTExpr {
public:
    ASTCallableCall(const std::shared_ptr<ASTExpr> &value, const ASTArguments &args,
                    const SourcePosition &p) : ASTExpr(p), callable_(value), args_(args) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
private:
    std::shared_ptr<ASTExpr> callable_;
    ASTArguments args_;
};

class ASTSuperMethod final : public ASTExpr {
public:
    ASTSuperMethod(const EmojicodeString &name, const ASTArguments &args, const SourcePosition &p)
    : ASTExpr(p), name_(name), args_(args) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
private:
    EmojicodeString name_;
    Type calleeType_ = Type::nothingness();
    ASTArguments args_;
};

class ASTCaptureMethod final : public ASTExpr {
public:
    ASTCaptureMethod(const EmojicodeString &name, const std::shared_ptr<ASTExpr> &callee, const SourcePosition &p)
    : ASTExpr(p), name_(name), callee_(callee) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
private:
    EmojicodeString name_;
    std::shared_ptr<ASTExpr> callee_;
};

class ASTCaptureTypeMethod final : public ASTExpr {
public:
    ASTCaptureTypeMethod(const EmojicodeString &name, const std::shared_ptr<ASTTypeExpr> &callee,
                         const SourcePosition &p) : ASTExpr(p), name_(name), callee_(callee) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
private:
    EmojicodeString name_;
    std::shared_ptr<ASTTypeExpr> callee_;
    bool contextedFunction_ = false;
};

class ASTTypeMethod final : public ASTExpr {
public:
    ASTTypeMethod(const EmojicodeString &name, const std::shared_ptr<ASTTypeExpr> &callee,
                  const ASTArguments &args, const SourcePosition &p)
    : ASTExpr(p), name_(name), callee_(callee), args_(args) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
private:
    bool valueType_ = false;
    EmojicodeString name_;
    const std::shared_ptr<ASTTypeExpr> callee_;
    ASTArguments args_;
};

class ASTConditionalAssignment final : public ASTExpr {
public:
    ASTConditionalAssignment(const EmojicodeString &varName, const std::shared_ptr<ASTExpr> &expr,
                             const SourcePosition &p) : ASTExpr(p), varName_(varName), expr_(expr) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
private:
    EmojicodeString varName_;
    std::shared_ptr<ASTExpr> expr_;
    VariableID varId_;
};
    
}

#endif /* ASTExpr_hpp */
