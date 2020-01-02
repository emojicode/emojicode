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
#include "Types/Type.hpp"
#include "MemoryFlowAnalysis/MFFlowCategory.hpp"
#include "Functions/Mood.hpp"
#include <utility>
#include <memory>

namespace llvm {
class Value;
}  // namespace llvm

namespace EmojicodeCompiler {

using llvm::Value;
class ASTTypeExpr;
class ExpressionAnalyser;
class TypeExpectation;
class FunctionCodeGenerator;
class Prettyprinter;
class MFFunctionAnalyser;
class ASTCall;
class ASTType;
class ASTReturn;

/// The superclass of all syntax tree nodes representing an expression.
///
/// The order in which every expression is visited is normally:
/// - `analyse` is called to determine a provisional expectation-independent result type
/// - `comply` is called to determine the final result type and allow adjustment to the expectation
/// - `analyseMemoryFlow` is called to analyse memory flow.
/// - `generate` is called to generate IR code.
class ASTExpr : public ASTNode {
    friend ExpressionAnalyser;
    friend ASTReturn;
public:
    explicit ASTExpr(const SourcePosition &p) : ASTNode(p) {}
    /// Set after semantic analysis and transformation.
    /// Iff this node represents an expression type this type is the exact type produced by this node.
    const Type& expressionType() const { return expressionType_; }

    /// Subclasses must override this method to generate IR.
    /// If the expression potentially evaluates to an managed value, handleResult() must be called.
    virtual Value* generate(FunctionCodeGenerator *fg) const = 0;
    virtual void analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) = 0;

    /// Informs this expression that if it creates a temporary object the object must not be released after the
    /// statement is executed. This method is called by MFFunctionAnalyser.
    void unsetIsTemporary() { isTemporary_ = false; unsetIsTemporaryPost(); }

    /// Informs this expresison that the reference it evaluates to is mutated.
    /// ASTExpr’s implementation does nothing. Subclasses can override this method.
    virtual void mutateReference(ExpressionAnalyser *analyser) {}

    /// Whether this expression produces a temporary value that must be released.
    /// If this returns true, the expression provides its result to FunctionCodeGenerator::addTemporaryObject at CG.
    /// @pre Call only after Memory Flow Analysis.
    bool producesTemporaryObject() const;

protected:
    /// This method must be called for every value that is created by the expression and must potentially be released.
    ///
    /// If the expression is temporary and expressionType() is a managed type, adds the value to the
    /// FunctionCodeGenerator::addTemporaryObject. References are never added.
    ///
    /// @param result The value produced by the expression. This can be `nullptr` if `vtReference` is provided.
    /// @param vtReference If the value is managed by reference, optionally provide a pointer to the value, so no
    ///                    temporary heap space must be allocated. If this value cannot be provided, provide `nullptr`.
    /// @return Always `result`.
    llvm::Value* handleResult(FunctionCodeGenerator *fg, llvm::Value *result, llvm::Value *vtReference = nullptr) const;

    /// This method is called at the end of unsetIsTemporary(). It can be overridden to perform additional tasks.
    virtual void unsetIsTemporaryPost() {}

    /// If the value created by evaluating the expression is a temporary value.
    /// @see MFFunctionAnalyser for a detailed explanation.
    bool isTemporary() const { return isTemporary_; }

    virtual Type analyse(ExpressionAnalyser *analyser) = 0;
    virtual Type comply(ExpressionAnalyser *analyser, const TypeExpectation &expectation) { return expressionType(); }
    void setExpressionType(const Type &type) { expressionType_ = type; }

private:
    bool isTemporary_ = true;
    Type expressionType_ = Type::invalid();
};

/// All expressions that represent a call (method, initializer, callable) that potentially raises an error inherit
/// from this class.
class ASTCall : public ASTExpr {
public:
    explicit ASTCall(const SourcePosition &p) : ASTExpr(p) {}

    /// Returns the error type or no return if the call cannot result in an error.
    virtual const Type& errorType() const = 0;

    virtual bool isErrorProne() const = 0;

    /// Informs the expression that a possible error return is dealt with.
    void setHandledError() { handledError_ = true; }

    void setErrorPointer(llvm::Value *errorDest) { errorDest_ = errorDest; }

protected:
    void ensureErrorIsHandled(ExpressionAnalyser *analyser) const;
    llvm::Value* errorPointer() const { return errorDest_; }

private:
    bool handledError_ = false;
    llvm::Value *errorDest_ = nullptr;
};

template<typename T, typename ...Args>
std::shared_ptr<T> insertNode(std::shared_ptr<ASTExpr> *node, Args&&... args) {
    auto pos = (*node)->position();
    *node = std::make_shared<T>(std::move(*node), pos, std::forward<Args>(args)...);
    return std::static_pointer_cast<T>(*node);
}

class ASTSizeOf final : public ASTExpr {
public:
    ASTSizeOf(std::unique_ptr<ASTType> type, const SourcePosition &p);
    Type analyse(ExpressionAnalyser *analyser) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) override {}

    ~ASTSizeOf();

private:
    std::unique_ptr<ASTType> type_;
};

class ASTArguments final : public ASTNode {
public:
    explicit ASTArguments(const SourcePosition &p);
    ASTArguments(const SourcePosition &p, std::vector<std::shared_ptr<ASTExpr>> args);
    ASTArguments(const SourcePosition &p, Mood mood);

    void addGenericArgument(std::unique_ptr<ASTType> type);
    const std::vector<std::shared_ptr<ASTType>>& genericArguments() const { return genericArguments_; }
    std::vector<std::shared_ptr<ASTType>>& genericArguments() { return genericArguments_; }
    void addArguments(const std::shared_ptr<ASTExpr> &arg) { arguments_.emplace_back(arg); }
    std::vector<std::shared_ptr<ASTExpr>>& args() { return arguments_; }
    const std::vector<std::shared_ptr<ASTExpr>>& args() const { return arguments_; }
    void toCode(PrettyStream &pretty) const;
    void genericArgsToCode(PrettyStream &pretty) const;
    Mood mood() const { return mood_; }
    void setMood(Mood mood) { mood_ = mood; }

    const std::vector<Type>& genericArgumentTypes() const { return genericArgumentsTypes_; }
    void setGenericArgumentTypes(std::vector<Type> types) { genericArgumentsTypes_ = std::move(types); }

    ~ASTArguments();

private:
    Mood mood_ = Mood::Imperative;
    std::vector<std::shared_ptr<ASTType>> genericArguments_;
    std::vector<std::shared_ptr<ASTExpr>> arguments_;
    std::vector<Type> genericArgumentsTypes_;
};

class ASTCallableCall final : public ASTCall {
public:
    ASTCallableCall(std::shared_ptr<ASTExpr> value, ASTArguments args,
                    const SourcePosition &p) : ASTCall(p), callable_(std::move(value)), args_(std::move(args)) {}
    Type analyse(ExpressionAnalyser *analyser) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override;

    const Type& errorType() const override { return callable_->expressionType().errorType(); }
    bool isErrorProne() const override { return callable_->expressionType().errorType().type() != TypeType::NoReturn; }

private:
    std::shared_ptr<ASTExpr> callable_;
    ASTArguments args_;
};

} // namespace EmojicodeCompiler

#endif /* ASTExpr_hpp */
