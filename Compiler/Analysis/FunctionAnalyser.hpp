//
//  FunctionAnalyser.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef FunctionAnalyser_hpp
#define FunctionAnalyser_hpp

#include "PathAnalyser.hpp"
#include "Types/TypeContext.hpp"
#include <memory>

namespace EmojicodeCompiler {

class ASTArguments;
class ASTBlock;
class TypeExpectation;
class SemanticAnalyser;
class SemanticScoper;
class Package;
class Compiler;
class ASTExpr;
struct SourcePosition;

/// This class is responsible for managing the semantic analysis of a function.
class FunctionAnalyser {
public:
    FunctionAnalyser(Function *function, SemanticAnalyser *analyser);

    FunctionAnalyser(Function *function, std::unique_ptr<SemanticScoper> scoper, SemanticAnalyser *analyser);

    void analyse();

    PathAnalyser& pathAnalyser() { return pathAnalyser_; }
    SemanticAnalyser* semanticAnalyser() const { return analyser_; }
    SemanticScoper& scoper() { return *scoper_; }
    const TypeContext& typeContext() const { return typeContext_; }
    Function* function() const { return function_; }
    Compiler* compiler() const;

    Type integer();
    Type boolean();
    Type real();
    Type symbol();
    Type byte();

    void setInUnsafeBlock(bool v) { inUnsafeBlock_ = v; }
    bool isInUnsafeBlock() const { return inUnsafeBlock_; }

    /// Ensures that the instance is fully initialized, which is required before using $this$ etc.
    void checkThisUse(const SourcePosition &p) const;

    /// Parses an expression node, verifies it return type and boxes it according to the given expectation.
    /// Calls @c expect internally.
    Type expectType(const Type &type, std::shared_ptr<ASTExpr>*, std::vector<CommonTypeFinder> *ctargs = nullptr);
    /// Parses an expression node and boxes it according to the given expectation. Calls @c box internally.
    Type expect(const TypeExpectation &expectation, std::shared_ptr<ASTExpr>*);
    /// Makes the node comply with the expectation by dereferencing, temporarily storing or boxing it.
    /// @param node A pointer to the node pointer. The pointer to which this pointer points might be changed.
    /// @note Only use this if there is a good reason why expect() cannot be used.
    Type comply(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node);

    /// Analyses @c node and sets the expression type of the node to the type that will be returned.
    /// @returns The type denoted by the $type-expression$ resolved by Type::resolveOnSuperArgumentsAndConstraints.
    Type analyseTypeExpr(const std::shared_ptr<ASTExpr> &node, const TypeExpectation &exp);

    Type analyseFunctionCall(ASTArguments *node, const Type &type, Function *function);

    ~FunctionAnalyser();

private:
    PathAnalyser pathAnalyser_;
    /// The scoper responsible for scoping the function being compiled.
    std::unique_ptr<SemanticScoper> scoper_;
    TypeContext typeContext_;

    Function *function_;
    SemanticAnalyser *analyser_;

    bool inUnsafeBlock_;

    /// Issues a warning at the given position if the function is deprecated.
    void deprecatedWarning(Function *function, const SourcePosition &p) const;

    void analyseReturn(ASTBlock *root);
    void analyseInitializationRequirements();

    /// Ensures that node has the required number of generic arguments for a call to function.
    /// If none are provided but function expects generic arguments, this method tries to infer them.
    void ensureGenericArguments(ASTArguments *node, const Type &type, Function *function);

    /// Checks that the function can be accessed or issues an error. Checks that the function is not deprecated
    /// and issues a warning otherwise.
    void checkFunctionUse(Function *function, const SourcePosition &p) const;

    /// Returns true if exprType and expectation are callables and there is a mismatch between the argument or return
    /// StorageTypes.
    bool callableBoxingRequired(const TypeExpectation &expectation, const Type &exprType);
    /// Boxes or unboxes the value to the StorageType specified by expectation.simplifyType()
    Type box(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node);
    /// Callable boxes the value if callableBoxingRequired() returns true.
    Type callableBox(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node);

    void makeIntoBox(Type &exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) const;

    void makeIntoSimple(Type &exprType, std::shared_ptr<ASTExpr> *node) const;

    void makeIntoSimpleOptional(Type &exprType, std::shared_ptr<ASTExpr> *node) const;

    void checkFunctionSafety(Function *function, const SourcePosition &p) const;

    void makeIntoSimpleError(Type &exprType, std::shared_ptr<ASTExpr> *node, const TypeExpectation &exp);

    void upcast(Type &exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node);
};

}  // namespace EmojicodeCompiler

#endif /* FunctionAnalyser_hpp */
