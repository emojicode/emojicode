//
//  ASTExpr.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTExpr_hpp
#define ASTExpr_hpp

#include <llvm/IR/Value.h>
#include "ASTNode.hpp"
#include <utility>

namespace EmojicodeCompiler {

using llvm::Value;
class ASTTypeExpr;
class SemanticAnalyser;
class TypeExpectation;
class FunctionCodeGenerator;
class Prettyprinter;

class ASTExpr : public ASTNode {
public:
    explicit ASTExpr(const SourcePosition &p) : ASTNode(p) {}
    /// Set after semantic analysis and transformation.
    /// Iff this node represents an expression type this type is the exact type produced by this node.
    const Type& expressionType() const { return expressionType_; }
    void setExpressionType(const Type &type) { expressionType_ = type; }
    void setTemporarilyScoped() { temporarilyScoped_ = true; }

    virtual Value* generate(FunctionCodeGenerator *fg) const = 0;
    virtual Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) = 0;
    virtual void toCode(Prettyprinter &pretty) const = 0;
private:
    Type expressionType_ = Type::noReturn();
    bool temporarilyScoped_ = false;
};
    
class ASTMetaTypeInstantiation final : public ASTExpr {
public:
    ASTMetaTypeInstantiation(Type type, const SourcePosition &p) : ASTExpr(p), type_(std::move(type)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    Type type_;
};

class ASTSizeOf final : public ASTExpr {
public:
    ASTSizeOf(Type type, const SourcePosition &p) : ASTExpr(p), type_(std::move(type)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    Type type_;
};

class ASTArguments final : public ASTNode {
public:
    explicit ASTArguments(const SourcePosition &p) : ASTNode(p) {}
    void addGenericArgument(const Type &type) { genericArguments_.emplace_back(type); }
    std::vector<Type>& genericArguments() { return genericArguments_; }
    void addArguments(const std::shared_ptr<ASTExpr> &arg) { arguments_.emplace_back(arg); }
    std::vector<std::shared_ptr<ASTExpr>>& arguments() { return arguments_; }
    const std::vector<std::shared_ptr<ASTExpr>>& arguments() const { return arguments_; }
    void toCode(Prettyprinter &pretty) const;
    bool isImperative() const { return imperative_; }
    void setImperative(bool imperative) { imperative_ = imperative; }
private:
    bool imperative_ = true;
    std::vector<Type> genericArguments_;
    std::vector<std::shared_ptr<ASTExpr>> arguments_;
};

class ASTCast final : public ASTExpr {
public:
    ASTCast(std::shared_ptr<ASTExpr> value, std::shared_ptr<ASTTypeExpr> type,
            const SourcePosition &p) : ASTExpr(p), value_(std::move(value)), typeExpr_(std::move(type)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    enum class CastType {
        ClassDowncast, ToClass, ToProtocol, ToValueType,
    };
    CastType castType_;
    std::shared_ptr<ASTExpr> value_;
    std::shared_ptr<ASTTypeExpr> typeExpr_;
    Value* downcast(FunctionCodeGenerator *fg) const;
    Value* castToClass(FunctionCodeGenerator *fg, Value *box) const;
    Value* castToValueType(FunctionCodeGenerator *fg, Value *box) const;
};

class ASTCallableCall final : public ASTExpr {
public:
    ASTCallableCall(std::shared_ptr<ASTExpr> value, ASTArguments args,
                    const SourcePosition &p) : ASTExpr(p), callable_(std::move(value)), args_(std::move(args)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    std::shared_ptr<ASTExpr> callable_;
    ASTArguments args_;
};

class ASTSuperMethod final : public ASTExpr {
public:
    ASTSuperMethod(std::u32string name, ASTArguments args, const SourcePosition &p)
    : ASTExpr(p), name_(std::move(name)), args_(std::move(args)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    std::u32string name_;
    Type calleeType_ = Type::noReturn();
    ASTArguments args_;
};

class ASTTypeMethod final : public ASTExpr {
public:
    ASTTypeMethod(std::u32string name, std::shared_ptr<ASTTypeExpr> callee,
                  ASTArguments args, const SourcePosition &p)
    : ASTExpr(p), name_(std::move(name)), callee_(std::move(callee)), args_(std::move(args)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    bool valueType_ = false;
    std::u32string name_;
    const std::shared_ptr<ASTTypeExpr> callee_;
    ASTArguments args_;
};

class ASTConditionalAssignment final : public ASTExpr {
public:
    ASTConditionalAssignment(std::u32string varName, std::shared_ptr<ASTExpr> expr,
                             const SourcePosition &p) : ASTExpr(p), varName_(std::move(varName)), expr_(std::move(expr)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    std::u32string varName_;
    std::shared_ptr<ASTExpr> expr_;
    VariableID varId_;
};
    
} // namespace EmojicodeCompiler

#endif /* ASTExpr_hpp */
