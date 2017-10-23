//
//  SemanticAnalyser.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef SemanticAnalyser_hpp
#define SemanticAnalyser_hpp

#include "../AST/ASTExpr.hpp"
#include "../AST/ASTTypeExpr.hpp"
#include "../CompilerError.hpp"
#include "../Scoping/SemanticScoper.hpp"
#include "../Types/Type.hpp"
#include "../Types/TypeContext.hpp"
#include "../Types/TypeExpectation.hpp"
#include "../Package/Package.hpp"
#include "PathAnalyser.hpp"
#include <memory>
#include <utility>

namespace EmojicodeCompiler {

class ASTArguments;

class SemanticAnalyser {
public:
    explicit SemanticAnalyser(Function *function)
    : scoper_(std::make_unique<SemanticScoper>(SemanticScoper::scoperForFunction(function))),
    typeContext_(function->typeContext()), function_(function) {}
    SemanticAnalyser(Function *function, std::unique_ptr<SemanticScoper> scoper)
    : scoper_(std::move(scoper)), typeContext_(function->typeContext()), function_(function) {}
    void analyse();

    PathAnalyser& pathAnalyser() { return pathAnalyser_; }
    SemanticScoper& scoper() { return *scoper_; }
    const TypeContext& typeContext() const { return typeContext_; }
    Function* function() { return function_; }
    Compiler* compiler() { return function_->package()->compiler(); }

    /// Parses an expression node, verifies it return type and boxes it according to the given expectation.
    /// Calls @c expect internally.
    Type expectType(const Type &type, std::shared_ptr<ASTExpr>*, std::vector<CommonTypeFinder> *ctargs = nullptr);
    /// Parses an expression node and boxes it according to the given expectation. Calls @c box internally.
    Type expect(const TypeExpectation &expectation, std::shared_ptr<ASTExpr>*);
    /// Makes the node comply with the expectation by dereferencing, temporarily storing or boxing it.
    /// @param node A pointer to the node pointer. The pointer to which this pointer points might be changed.
    /// @note Only use this if there is a good reason why expect() cannot be used.
    Type comply(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node);

    void validateMetability(const Type &originalType, const SourcePosition &p) const {
        if (!originalType.allowsMetaType()) {
            throw CompilerError(p, "Metatype of ", originalType.toString(typeContext_), " is not available.");
        }
    }

    Type analyseTypeExpr(const std::shared_ptr<ASTTypeExpr> &node, const TypeExpectation &exp) {
        auto type = node->analyse(this, exp).resolveOnSuperArgumentsAndConstraints(typeContext_);
        node->setExpressionType(type);
        return type;
    }

    bool typeIsEnumerable(const Type &type, Type *elementType);

    void validateMethodCapturability(const Type &type, const SourcePosition &p) const {
        if (type.type() == TypeType::ValueType) {
            if (type.size() > 1) {
                throw CompilerError(p, "Type not eligible for method capturing.");
            }
        }
        else if (type.type() != TypeType::Class) {
            throw CompilerError(p, "You can’t capture method calls on this kind of type.");
        }
    }

    void validateAccessLevel(Function *function, const SourcePosition &p) const;

    Type analyseFunctionCall(ASTArguments *node, const Type &type, Function *function);
private:
    PathAnalyser pathAnalyser_;
    /// The scoper responsible for scoping the function being compiled.
    std::unique_ptr<SemanticScoper> scoper_;
    TypeContext typeContext_;

    Function *function_;

    void analyseReturn(const std::shared_ptr<ASTBlock> &);
    void analyseInitializationRequirements();

    /// Ensures that node has the required number of generic arguments for a call to function.
    /// If none are provided but function expects generic arguments, this method tries to infer them.
    void ensureGenericArguments(ASTArguments *node, const Type &type, Function *function);

    /// Returns true if exprType and expectation are callables and there is a mismatch between the argument or return
    /// StorageTypes.
    bool callableBoxingRequired(const TypeExpectation &expectation, const Type &exprType);
    /// Boxes or unboxes the value to the StorageType specified by expectation.simplifyType()
    Type box(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node);
    /// Callable boxes the value if callableBoxingRequired() returns true.
    Type callableBox(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node);
};

}  // namespace EmojicodeCompiler

#endif /* SemanticAnalyser_hpp */
