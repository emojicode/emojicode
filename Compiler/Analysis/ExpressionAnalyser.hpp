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
class CompilerError;
class ASTTypeExpr;

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
    const PathAnalyser& pathAnalyser() const { return pathAnalyser_; }
    Package* package() const { return package_; }
    SemanticScoper& scoper() { return *scoper_; }
    const SemanticScoper& scoper() const { return *scoper_; }

    void error(const CompilerError &ce) const;

    virtual bool isInUnsafeBlock() const { return false; }

    /// Ensures that the instance is fully initialized, which is required before using $this$ etc.
    ///
    /// ExpressionAnalyser’s implementation is empty as ExpressionAnalyser itself cannot analyse an expression in
    /// a context with this.
    virtual void checkThisUse(const SourcePosition &p) const {}

    virtual void configureClosure(Function *closure) const;

    virtual FunctionType functionType() const;

    Type analyse(const std::shared_ptr<ASTExpr> &ptr);
    /// Parses an expression node, verifies it return type and boxes it according to the given expectation.
    /// Calls @c expect internally.
    void expectType(const Type &type, std::shared_ptr<ASTExpr>*);
    /// Parses an expression node and boxes it according to the given expectation. Calls @c box internally.
    Type expect(const TypeExpectation &expectation, std::shared_ptr<ASTExpr>*);
    /// Makes the node comply with the expectation by dereferencing, temporarily storing or boxing it.
    /// @param node A pointer to the node pointer. The pointer to which this pointer points might be changed.
    /// @note Only use this if there is a good reason why expect() cannot be used.
    Type comply(const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node);

    /// Analyses @c node and sets the expression type of the node to the type that will be returned.
    /// @returns The type denoted by the $type-expression$ resolved by Type::resolveOnSuperArgumentsAndConstraints.
    /// @param allowGenericInf If true, generic arguments must not be specified. The type will not contain any arguments
    /// in that case and must be inferred with analyseFunctionCall().
    Type analyseTypeExpr(const std::shared_ptr<ASTTypeExpr> &node, const TypeExpectation &exp,
                         bool allowGenericInf = false);

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

    Type upcast(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) const;

    Type complyReference(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) const;
};

}  // namespace EmojicodeCompiler

#endif /* ExpressionAnalyser_hpp */
