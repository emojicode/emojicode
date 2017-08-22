//
//  SemanticAnalyser.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef SemanticAnalyser_hpp
#define SemanticAnalyser_hpp

#include "../AST/ASTStatements.hpp"
#include "../AST/ASTExpr.hpp"
#include "../AST/ASTTypeExpr.hpp"
#include "PathAnalyser.hpp"
#include "../Scoping/SemanticScoper.hpp"
#include "../Types/Type.hpp"
#include "../Types/TypeExpectation.hpp"
#include "../Types/TypeContext.hpp"
#include "../FunctionType.hpp"
#include "../CompilerError.hpp"
#include <utility>
#include <memory>

namespace EmojicodeCompiler {

class ASTArguments;

class SemanticAnalyser {
public:
    SemanticAnalyser(Function *function)
    : scoper_(std::make_unique<SemanticScoper>(SemanticScoper::scoperForFunction(function))),
    typeContext_(function->typeContext()), function_(function) {}
    SemanticAnalyser(Function *function, std::unique_ptr<SemanticScoper> scoper)
    : scoper_(std::move(scoper)), typeContext_(function->typeContext()), function_(function) {}
    void analyse();

    PathAnalyser& pathAnalyser() { return pathAnalyser_; }
    SemanticScoper& scoper() { return *scoper_; }
    const TypeContext& typeContext() const { return typeContext_; }
    Function* function() { return function_; }

    /// Parses an expression node, verifies it return type and boxes it according to the given expectation.
    /// Calls @c expect internally.
    Type expectType(const Type &type, std::shared_ptr<ASTExpr>*, std::vector<CommonTypeFinder> *ctargs = nullptr);
    /// Parses an expression node and boxes it according to the given expectation. Calls @c box internally.
    Type expect(const TypeExpectation &expectation, std::shared_ptr<ASTExpr>*);
    /// Performs boxing on a node exactly as @c expect does. Only use this if @c expect has been ruled out.
    Type box(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node);

    void validateMetability(const Type &originalType, const SourcePosition &p) const {
        if (!originalType.allowsMetaType()) {
            auto string = originalType.toString(typeContext_);
            throw CompilerError(p, "Can’t get metatype of %s.", string.c_str());
        }
    }

    /// Tries to pop the temporary scope (see SemanticScoper) and wraps the node into a @c ASTTemporarilyScoped
    // if necessary.
    void popTemporaryScope(const std::shared_ptr<ASTExpr> &node) {
        if (scoper_->popTemporaryScope()) {
            node->setTemporarilyScoped();
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

    bool callableBoxingRequired(const TypeExpectation &expectation, const Type &exprType);
};

}  // namespace Emojicode

#endif /* SemanticAnalyser_hpp */
