//
//  CallableParserAndGenerator.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CallableParserAndGenerator_hpp
#define CallableParserAndGenerator_hpp

#include <utility>
#include <vector>
#include <functional>
#include "EmojicodeCompiler.hpp"
#include "Function.hpp"
#include "CallableScoper.hpp"
#include "AbstractParser.hpp"
#include "TypeContext.hpp"
#include "CallableWriter.hpp"
#include "CallableParserAndGeneratorMode.hpp"
#include "Destination.hpp"

struct FlowControlReturn {
    int branches = 0;
    int branchReturns = 0;

    bool returned() { return branches == branchReturns; }
};

enum class TypeAvailability {
    /** The type is known at compile type, can’t change and will be available at runtime. Instruction to retrieve the
     type at runtime were written to the file. */
    StaticAndAvailabale,
    /** The type is known at compile type, but a another compatible type might be provided at runtime instead.
     The type will be available at runtime. Instruction to retrieve the type at runtime were written to the file. */
    DynamicAndAvailabale,
    /** The type is fully known at compile type but won’t be available at runtime (Enum, ValueType etc.).
     Nothing was written to the file if this value is returned. */
    StaticAndUnavailable,
};

/** This class is repsonsible for compiling a @c Callable to bytecode. */
class CallableParserAndGenerator : AbstractParser {
public:
    static void writeAndAnalyzeFunction(Function *function, CallableWriter &writer, Type contextType,
                                        CallableScoper &scoper, CallableParserAndGeneratorMode mode);
    CallableParserAndGenerator(Callable &callable, Package *p, CallableParserAndGeneratorMode mode,
                               TypeContext typeContext, CallableWriter &writer, CallableScoper &scoper);

    /** Performs the analyziation. */
    void analyze();
    /** Whether self was used in the callable body. */
    bool usedSelfInBody() const { return usedSelf; }

    static bool hasInstanceScope(CallableParserAndGeneratorMode mode);
private:
    CallableParserAndGeneratorMode mode;
    /** The callable which is processed. */
    Callable &callable;
    /** The writer used for writing the byte code. */
    CallableWriter &writer;
    /** The scoper responsible for scoping the function being compiled. */
    CallableScoper &scoper;

    /** The flow control depth. */
    int flowControlDepth = 0;
    /** Whether the statment has an effect. */
    bool effect = false;
    /** Whether the function in compilation has returned. */
    bool returned = false;
    /** Whether the this context or an instance variable has been acessed. */
    bool usedSelf = false;
    /** Whether the superinitializer has been called (always false if the function is not an intializer). */
    bool calledSuper = false;

    /** The this context in which this function operates. */
    TypeContext typeContext;

    void parseStatement(const Token &token);

    Type parse(const Token &token, Type expectation, Destination &des);
    Type parse(const Token &token, Destination &des);
    /**
     * Same as @c parse. This method however forces the returned type to be a type compatible to @c type.
     * @param token The token to evaluate.
     */
    Type parse(const Token &token, const Token &parentToken, Type type, Destination des,
               std::vector<CommonTypeFinder>* = nullptr);
    /**
     * Parses a type name and writes instructions to fetch it at runtime. If everything goes well, the parsed
     * type (unresolved) and true are returned.
     * If the type, however, isn’t available at runtime (Enum, ValueType, Protocol) the parsed type (unresolved)
     * is returned and false are returned.
     */
    std::pair<Type, TypeAvailability> parseTypeAsValue(TypeContext tc, SourcePosition p,
                                                       Type expectation = Type::nothingness());
    /** Parses an identifier when occurring without context. */
    Type parseIdentifier(const Token &token, Type expectation, Destination &des);
    /** Parses the expression for an if statement. */
    void parseIfExpression(const Token &token);
    /**
     * Parses a function call. This method takes care of parsing all arguments as well as generic arguments and of
     * infering them if necessary.
     * @param type The type for the type context, therefore the type on which this function was called.
     */
    Type parseFunctionCall(Type type, Function *p, const Token &token);

    /// Writes instructions necessary to peform an action on a variable.
    /// @param inInstanceScope Whether the value is in the instance or on the stack.
    /// @param stack The instruction for a stack variable.
    /// @param object The instruction for an object instance variable.
    /// @param vt The instruction for a value type instance variable.
    void writeInstructionForStackOrInstance(bool inInstanceScope, EmojicodeInstruction stack,
                                            EmojicodeInstruction object, EmojicodeInstruction vt, SourcePosition p);

    /// Writes the instructions necessary to wrap or unwrap in order to produce correctly to the given destination, and
    /// the instructions to store the return value at a temporary location on the stack, if the destination type of
    /// temporary type.
    /// @attention Makes @c returnType a reference if the returned value type is temporarily needed as reference.
    void writeBoxingAndTemporary(Destination des, Type &returnType, SourcePosition p, WriteLocation location) const;
    void writeBoxingAndTemporary(Destination des, Type returnType, SourcePosition p) const {
        writeBoxingAndTemporary(des, returnType, p, writer);
    }

    Type parseMethodCall(const Token &token, Destination des, std::function<Type(Destination&)> callee);

    /// Copies or takes a reference to the content of the given variable as needed to statisfy the requirement of @c des
    /// @returns The type of the variable with @c isValueReference set appropriately.
    Type takeVariable(ResolvedVariable rvar, Destination &des, SourcePosition p);
    void copyVariableContent(ResolvedVariable var, SourcePosition p);
    void getVTReference(ResolvedVariable var, SourcePosition p);
    void produceToVariable(ResolvedVariable var, SourcePosition p);

    void noReturnError(SourcePosition p);
    void noEffectWarning(const Token &warningToken);
    void mutatingMethodCheck(Function *function, Type type, Destination des, SourcePosition p);
    bool typeIsEnumerable(Type type, Type *elementType);
    void flowControlBlock(bool block = true, std::function<void()> = nullptr);

    void flowControlReturnEnd(FlowControlReturn &fcr);

    bool isSuperconstructorRequired() const;
    bool isFullyInitializedCheckRequired() const;
    bool isSelfAllowed() const;
    bool hasInstanceScope() const;
    bool isOnlyNothingnessReturnAllowed() const;

    void notStaticError(TypeAvailability t, SourcePosition p, const char *name);
    bool isStatic(TypeAvailability t) { return t == TypeAvailability::StaticAndUnavailable
        || t == TypeAvailability::StaticAndAvailabale; }
};

#endif /* CallableParserAndGenerator_hpp */
