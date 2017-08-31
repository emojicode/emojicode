//
//  FunctionPAG.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef FunctionPAG_hpp
#define FunctionPAG_hpp

#include <utility>
#include <vector>
#include <functional>
#include "EmojicodeCompiler.hpp"
#include "Function.hpp"
#include "CallableScoper.hpp"
#include "AbstractParser.hpp"
#include "TypeContext.hpp"
#include "FunctionWriter.hpp"
#include "FunctionPAGMode.hpp"
#include "BoxingLayer.hpp"
#include "TypeExpectation.hpp"
#include "FunctionPAGInterface.hpp"
#include "TypeAvailability.hpp"
#include "PathAnalyser.hpp"

namespace EmojicodeCompiler {

class MigWriter;

/// This class is responsible to parse and compile (abbreviated PAG) all functions to byte code.
/// One instance of @c FunctionPAG can compile exactly one function.
class FunctionPAG final : AbstractParser, private FunctionPAGInterface {
public:
    FunctionPAG(Function &function, Type contextType, FunctionWriter &writer, CallableScoper &scoper, MigWriter *migWriter);

    /// Parses the function and compiles it to bytecode. The bytecode is appended to the writer.
    void compile();

    /** Whether self was used in the callable body. */
    bool usedSelfInBody() const { return usedSelf; }

    static bool hasInstanceScope(FunctionPAGMode mode);
private:
    /// The function to compile
    Function &function_;
    /// The writer to which the byte code will be written
    FunctionWriter &writer_;
    /// The scoper responsible for scoping the function being compiled.
    CallableScoper &scoper_;

    PathAnalyser pathAnalyser;

    MigWriter *migWriter_;

    FunctionWriter& writer() override { return writer_; }
    TokenStream& stream() override { return stream_; }
    ArgumentsMigCreator& argsMigCreator() override { return function_.creator; }
    Package* package() override { return package_; }
    FunctionPAGMode mode() const override { return function_.compilationMode(); }
    CallableScoper& scoper() override { return scoper_; }

    /// Whether the statment has an effect.
    bool effect = false;

    void makeEffective() override { effect = true; }

    /** Whether the this context or an instance variable has been acessed. */
    bool usedSelf = false;

    /** The this context in which this function operates. */
    TypeContext typeContext_;

    const TypeContext& typeContext() override { return typeContext_; }

    void parseStatement();

    Type parseExpr(TypeExpectation &&expectation) override {
        return parseExprToken(stream_.consumeToken(), std::move(expectation));
    }
    Type parseExprToken(const Token &token, TypeExpectation &&expectation);
    Type parseTypeSafeExpr(Type type, std::vector<CommonTypeFinder>* = nullptr) override;

    std::pair<Type, TypeAvailability> parseTypeAsValue(const SourcePosition &p,  const TypeExpectation &expectation) override;

    Scope& setArgumentVariables();
    void compileCode(Scope &methodScope);

    /// Parses an expression identifier
    Type parseExprIdentifier(const Token &token, const TypeExpectation &expectation);

    /// Parses a condition as used by if, while etc.
    FunctionWriter parseCondition(const Token &token, bool temporaryWriter);
    /**
     * Parses a function call. This method takes care of parsing all arguments as well as generic arguments and of
     * infering them if necessary.
     * @param type The type for the type context, therefore the type on which this function was called.
     */
    Type parseFunctionCall(const Type &type, Function *p, const Token &token) override;

    /// Writes instructions necessary to peform an action on a variable.
    /// @param inInstanceScope Whether the value is in the instance or on the stack.
    /// @param stack The instruction for a stack variable.
    /// @param object The instruction for an object instance variable.
    /// @param vt The instruction for a value type instance variable.
    void writeInstructionForStackOrInstance(bool inInstanceScope, EmojicodeInstruction stack,
                                            EmojicodeInstruction object, EmojicodeInstruction vt);

    /// Writes the instructions necessary to wrap or unwrap in order to produce correctly to the given destination, and
    /// the instructions to store the return value at a temporary location on the stack, if the destination type of
    /// temporary type.
    /// @attention Makes @c returnType a reference if the returned value type is temporarily needed as reference.
    void box(const TypeExpectation &expectation, Type &returnType, WriteLocation location) const override;
    void box(const TypeExpectation &expectation, Type returnType) const override {
        box(expectation, returnType, writer_);
    }

    Type parseMethodCall(const Token &token, const TypeExpectation &expectation,
                         std::function<Type(TypeExpectation&)> callee);

    /// Copies or takes a reference to the content of the given variable as needed to statisfy the requirement of @c des
    /// @returns The type of the variable with @c isReference set appropriately.
    Type takeVariable(ResolvedVariable rvar, const TypeExpectation &expectation);
    void copyVariableContent(ResolvedVariable var);
    void getVTReference(ResolvedVariable var);
    void produceToVariable(ResolvedVariable var);

    TypeContext typeContextForType(Type contextType) {
        if (contextType.type() == TypeContent::ValueType) {
            if (function_.mutating()) {
                contextType.setMutable(true);
            }
            contextType.setReference();
        }
        else if (contextType.type() == TypeContent::Enum) {
            contextType.setReference();
        }

        return TypeContext(contextType, &function_);
    }

    void noReturnError(const SourcePosition &p);
    void noEffectWarning(const Token &warningToken);
    void mutatingMethodCheck(Function *method, const Type &type, const TypeExpectation &expectation,
                             const SourcePosition &p);
    void checkAccessLevel(Function *function, const SourcePosition &p) const override;
    bool typeIsEnumerable(const Type &type, Type *elementType);
    void flowControlBlock(bool block = true, const std::function<void()> &bodyPredicate = nullptr);

    void generateBoxingLayer(BoxingLayer &layer);

    bool isSuperconstructorRequired() const;
    bool isFullyInitializedCheckRequired() const;
    bool isSelfAllowed() const;
    bool hasInstanceScope() const;
    bool isOnlyNothingnessReturnAllowed() const;

    Type parseTypeDeclarative(const TypeContext &typeContext, TypeDynamism dynamism,
                              Type expectation = Type::nothingness(), TypeDynamism *dynamicType = nullptr) override {
        return AbstractParser::parseTypeDeclarative(typeContext, dynamism, expectation, dynamicType);
    }
};

}  // namespace EmojicodeCompiler

#endif /* FunctionPAG_hpp */
