//
//  ASTExpr.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTExpr_hpp
#define ASTExpr_hpp

#include "ASTType.hpp"
#include "ASTNode.hpp"
#include "Types/Type.hpp"
#include "ErrorSelfDestructing.hpp"
#include "Scoping/Variable.hpp"
#include <llvm/IR/Value.h>
#include <utility>

namespace EmojicodeCompiler {

using llvm::Value;
class ASTTypeExpr;
class FunctionAnalyser;
class TypeExpectation;
class FunctionCodeGenerator;
class Prettyprinter;
class MFFunctionAnalyser;
enum class MFFlowCategory;

/// The superclass of all syntax tree nodes representing an expression.
class ASTExpr : public ASTNode {
public:
    explicit ASTExpr(const SourcePosition &p) : ASTNode(p) {}
    /// Set after semantic analysis and transformation.
    /// Iff this node represents an expression type this type is the exact type produced by this node.
    const Type& expressionType() const { return expressionType_; }
    void setExpressionType(const Type &type) { expressionType_ = type; }

    /// Subclasses must override this method to generate IR.
    /// If the expression potentially evaluates to an managed value, handleResult() must be called.
    virtual Value* generate(FunctionCodeGenerator *fg) const = 0;
    virtual Type analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) = 0;
    virtual void analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) = 0;

    /// Informs this expression that if it creates a temporary object the object must not be released after the
    /// statement is executed. This method is called by MFFunctionAnalyser.
    void unsetIsTemporary() { isTemporary_ = false; unsetIsTemporaryPost(); }

protected:
    /// Whether the object to which this expression evaluates should be released.
    llvm::Value* handleResult(FunctionCodeGenerator *fg, llvm::Value *result,
                              bool valueTypeIsReferenced = false) const;

    /// This method is called at the end of unsetIsTemporary(). It can be overridden to perform additional tasks.
    virtual void unsetIsTemporaryPost() {}

    /// If the value created by evaluating the expression is a temporary value.
    /// @see MFFunctionAnalyser for a detailed explanation.
    bool isTemporary() const { return isTemporary_; }

private:
    bool isTemporary_ = true;
    Type expressionType_ = Type::noReturn();
};

template<typename T, typename ...Args>
std::shared_ptr<T> insertNode(std::shared_ptr<ASTExpr> *node, Args&&... args) {
    auto pos = (*node)->position();
    *node = std::make_shared<T>(std::move(*node), pos, std::forward<Args>(args)...);
    return std::static_pointer_cast<T>(*node);
}

class ASTTypeAsValue final : public ASTExpr {
public:
    ASTTypeAsValue(std::unique_ptr<ASTType> type, TokenType tokenType, const SourcePosition &p)
        : ASTExpr(p), type_(std::move(type)), tokenType_(tokenType) {}
    Type analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) override {}

private:
    std::unique_ptr<ASTType> type_;
    TokenType tokenType_;
};

class ASTSizeOf final : public ASTExpr {
public:
    ASTSizeOf(std::unique_ptr<ASTType> type, const SourcePosition &p) : ASTExpr(p), type_(std::move(type)) {}
    Type analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) override {}

private:
    std::unique_ptr<ASTType> type_;
};

class ASTArguments final : public ASTNode {
public:
    explicit ASTArguments(const SourcePosition &p) : ASTNode(p) {}

    ASTArguments(const SourcePosition &p, bool imperative) : ASTNode(p), imperative_(imperative) {}
    void addGenericArgument(std::unique_ptr<ASTType> type) { genericArguments_.emplace_back(std::move(type)); }
    const std::vector<std::shared_ptr<ASTType>>& genericArguments() const { return genericArguments_; }
    std::vector<std::shared_ptr<ASTType>>& genericArguments() { return genericArguments_; }
    void addArguments(const std::shared_ptr<ASTExpr> &arg) { arguments_.emplace_back(arg); }
    std::vector<std::shared_ptr<ASTExpr>>& args() { return arguments_; }
    const std::vector<std::shared_ptr<ASTExpr>>& args() const { return arguments_; }
    void toCode(PrettyStream &pretty) const;
    bool isImperative() const { return imperative_; }
    void setImperative(bool imperative) { imperative_ = imperative; }

    const std::vector<Type>& genericArgumentTypes() const { return genericArgumentsTypes_; }
    void setGenericArgumentTypes(std::vector<Type> types) { genericArgumentsTypes_ = std::move(types); }

private:
    bool imperative_ = true;
    std::vector<std::shared_ptr<ASTType>> genericArguments_;
    std::vector<std::shared_ptr<ASTExpr>> arguments_;
    std::vector<Type> genericArgumentsTypes_;
};

class ASTCast final : public ASTExpr {
public:
    ASTCast(std::shared_ptr<ASTExpr> value, std::shared_ptr<ASTExpr> type,
            const SourcePosition &p) : ASTExpr(p), value_(std::move(value)), typeExpr_(std::move(type)) {}
    Type analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override;

private:
    enum class CastType {
        ClassDowncast, ToClass, ToProtocol, ToValueType,
    };
    CastType castType_;
    std::shared_ptr<ASTExpr> value_;
    std::shared_ptr<ASTExpr> typeExpr_;
    Value* downcast(FunctionCodeGenerator *fg) const;
    Value* castToClass(FunctionCodeGenerator *fg, Value *box) const;
    Value* castToValueType(FunctionCodeGenerator *fg, Value *box) const;
    Value* castToProtocol(FunctionCodeGenerator *fg, Value *box) const;
    /// Returns the box info representing the type of information in the box. This includes fetching the box info
    /// from the protocol conformance if the box is a protocol box.
    Value* boxInfo(FunctionCodeGenerator *fg, Value *box) const;
};

class ASTCallableCall final : public ASTExpr {
public:
    ASTCallableCall(std::shared_ptr<ASTExpr> value, ASTArguments args,
                    const SourcePosition &p) : ASTExpr(p), callable_(std::move(value)), args_(std::move(args)) {}
    Type analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override;

private:
    std::shared_ptr<ASTExpr> callable_;
    ASTArguments args_;
};

class ASTSuper final : public ASTExpr, private ErrorSelfDestructing {
public:
    ASTSuper(std::u32string name, ASTArguments args, const SourcePosition &p)
    : ASTExpr(p), name_(std::move(name)), args_(std::move(args)) {}
    Type analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override;

private:
    void analyseSuperInit(FunctionAnalyser *analyser);
    std::u32string name_;
    Function *function_ = nullptr;
    Type calleeType_ = Type::noReturn();
    ASTArguments args_;
    bool init_ = false;
    bool manageErrorProneness_ = false;

    void analyseSuperInitErrorProneness(FunctionAnalyser *analyser, const Initializer *initializer);
};

class ASTConditionalAssignment final : public ASTExpr {
public:
    ASTConditionalAssignment(std::u32string varName, std::shared_ptr<ASTExpr> expr,
                             const SourcePosition &p) : ASTExpr(p), varName_(std::move(varName)), expr_(std::move(expr)) {}
    Type analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override;

private:
    std::u32string varName_;
    std::shared_ptr<ASTExpr> expr_;
    VariableID varId_;
};
    
} // namespace EmojicodeCompiler

#endif /* ASTExpr_hpp */
