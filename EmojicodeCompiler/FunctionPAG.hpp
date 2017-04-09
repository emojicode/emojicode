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

/// This class is responsible to parse and compile (abbreviated PAG) all functions to byte code.
/// One instance of @c FunctionPAG can compile exactly one function.
class FunctionPAG : AbstractParser {
public:
    FunctionPAG(Function &function, Type contextType, FunctionWriter &writer, CallableScoper &scoper);
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

    Type parse(const Token &token, TypeExpectation &&expectation);
    /**
     * Same as @c parse. This method however forces the returned type to be a type compatible to @c type.
     * @param token The token to evaluate.
     */
    Type parse(const Token &token, const Token &parentToken, Type type, std::vector<CommonTypeFinder>* = nullptr);
    /**
     * Parses a type name and writes instructions to fetch it at runtime. If everything goes well, the parsed
     * type (unresolved) and true are returned.
     * If the type, however, isn’t available at runtime (Enum, ValueType, Protocol) the parsed type (unresolved)
     * is returned and false are returned.
     */
    std::pair<Type, TypeAvailability> parseTypeAsValue(const SourcePosition &p,  const TypeExpectation &expectation);
    
    /** Parses an identifier when occurring without context. */
    Type parseIdentifier(const Token &token, const TypeExpectation &expectation);
    /// Parses a condition as used by if, while etc.
    FunctionWriter parseCondition(const Token &token, bool temporaryWriter);
    /**
     * Parses a function call. This method takes care of parsing all arguments as well as generic arguments and of
     * infering them if necessary.
     * @param type The type for the type context, therefore the type on which this function was called.
     */
    Type parseFunctionCall(const Type &type, Function *p, const Token &token);

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
    void writeBoxingAndTemporary(const TypeExpectation &expectation, Type &returnType, WriteLocation location) const;
    void writeBoxingAndTemporary(const TypeExpectation &expectation, Type returnType) const {
        writeBoxingAndTemporary(expectation, returnType, writer_);
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
    void checkAccessLevel(Function *function, const SourcePosition &p) const;
    bool typeIsEnumerable(const Type &type, Type *elementType);
    void flowControlBlock(bool block = true, const std::function<void()> &bodyPredicate = nullptr);

    void generateBoxingLayer(BoxingLayer &layer);

    void flowControlReturnEnd(FlowControlReturn &fcr);

    bool isSuperconstructorRequired() const;
    bool isFullyInitializedCheckRequired() const;
    bool isSelfAllowed() const;
    bool hasInstanceScope() const;
    bool isOnlyNothingnessReturnAllowed() const;

    void notStaticError(TypeAvailability t, const SourcePosition &p, const char *name);
    bool isStatic(TypeAvailability t) { return t == TypeAvailability::StaticAndUnavailable
        || t == TypeAvailability::StaticAndAvailabale; }
};

#endif /* FunctionPAG_hpp */
