//
//  ExpressionAnalyser.hpp
//  runtime
//
//  Created by Theo Weidmann on 28.11.18.
//

#ifndef ExpressionAnalyser_hpp
#define ExpressionAnalyser_hpp

#include "PathAnalyser.hpp"
#include "Types/TypeContext.hpp"
#include <memory>

namespace EmojicodeCompiler {

class Compiler;
class SemanticAnalyser;
enum class FunctionType;
class SemanticScoper;
class TypeExpectation;
class ASTExpr;
struct SourcePosition;
class ASTArguments;

/// This class provides all interfaces required for analysing an expression.
///
/// Although it can be directly instantiated, mostly its subclass FunctionAnalyser is used.
///
/// @see ASTExpr
class ExpressionAnalyser {
public:
    ExpressionAnalyser(SemanticAnalyser *analyser, TypeContext typeContext, Package *package,
                       std::unique_ptr<SemanticScoper> scoper);

    SemanticAnalyser* semanticAnalyser() const { return analyser_; }
    Compiler* compiler() const;
    const TypeContext& typeContext() const { return typeContext_; }
    PathAnalyser& pathAnalyser() { return pathAnalyser_; }
    Package* package() const { return package_; }
    SemanticScoper& scoper() { return *scoper_; }

    virtual bool isInUnsafeBlock() const { return false; }

    /// Ensures that the instance is fully initialized, which is required before using $this$ etc.
    ///
    /// ExpressionAnalyser’s implementation is empty as ExpressionAnalyser itself cannot analyse an expression in
    /// a context with this.
    virtual void checkThisUse(const SourcePosition &p) const {}

    virtual void configureClosure(Function *closure) const;

    virtual FunctionType functionType() const;

    /// Parses an expression node, verifies it return type and boxes it according to the given expectation.
    /// Calls @c expect internally.
    Type expectType(const Type &type, std::shared_ptr<ASTExpr>*, std::vector<CommonTypeFinder> *ctargs = nullptr);
    /// Parses an expression node and boxes it according to the given expectation. Calls @c box internally.
    Type expect(const TypeExpectation &expectation, std::shared_ptr<ASTExpr>*);
    /// Makes the node comply with the expectation by dereferencing, temporarily storing or boxing it.
    /// @param node A pointer to the node pointer. The pointer to which this pointer points might be changed.
    /// @note Only use this if there is a good reason why expect() cannot be used.
    Type comply(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node);

    /// Verifies the call of the provided functions with the provided arguments.
    /// @param type The type on which the provided method is called.
    ///
    /// This method:
    /// - Verifies the provided generic arguments or tries to infer them.
    /// - Verifies that the number and types of arguments is correct.
    /// - Issues a warning if the function is deprecated.
    /// - Ensures that access control allows this function to be called.
    /// - Checks that the function is safe or ensures that we are in an unsafe block.
    Type analyseFunctionCall(ASTArguments *node, const Type &type, Function *function);

    Type integer() const;
    Type boolean() const;
    Type real() const;
    Type byte() const;

    virtual ~ExpressionAnalyser();

protected:
    PathAnalyser pathAnalyser_;
    TypeContext typeContext_;
    SemanticAnalyser *analyser_;
    std::unique_ptr<SemanticScoper> scoper_;

private:
    Package *package_;

    /// Issues a warning at the given position if the function is deprecated.
    void deprecatedWarning(Function *function, const SourcePosition &p) const;

    /// Ensures that node has the required number of generic arguments for a call to function.
    /// If none are provided but function expects generic arguments, this method tries to infer them.
    void ensureGenericArguments(ASTArguments *node, const Type &type, Function *function);

    /// Checks that the function can be accessed or issues an error. Checks that the function is not deprecated
    /// and issues a warning otherwise.
    void checkFunctionUse(Function *function, const SourcePosition &p) const;

    /// Returns true if exprType and expectation are callables and there is a mismatch between the argument or return
    /// StorageTypes.
    bool callableBoxingRequired(const TypeExpectation &expectation, const Type &exprType) const;
    /// Boxes or unboxes the value to the StorageType specified by expectation.simplifyType()
    Type box(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) const;
    /// Callable boxes the value if callableBoxingRequired() returns true.
    Type callableBox(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) const;

    void makeIntoBox(Type &exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) const;

    void makeIntoSimple(Type &exprType, std::shared_ptr<ASTExpr> *node) const;

    void makeIntoSimpleOptional(Type &exprType, std::shared_ptr<ASTExpr> *node) const;

    void checkFunctionSafety(Function *function, const SourcePosition &p) const;

    void makeIntoSimpleError(Type &exprType, std::shared_ptr<ASTExpr> *node, const TypeExpectation &exp) const;

    Type upcast(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) const;

    Type complyReference(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) const;
};

}  // namespace EmojicodeCompiler

#endif /* ExpressionAnalyser_hpp */
