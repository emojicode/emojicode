//
//  CallableParserAndGenerator.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CallableParserAndGenerator_hpp
#define CallableParserAndGenerator_hpp

#include "EmojicodeCompiler.hpp"
#include "Function.hpp"
#include "CallableScoper.hpp"
#include "AbstractParser.hpp"
#include "TypeContext.hpp"
#include "CallableWriter.hpp"
#include "CallableParserAndGeneratorMode.hpp"

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
    static void writeAndAnalyzeFunction(Function *function, CallableWriter &writer, Type classType,
                                        CallableScoper &scoper, CallableParserAndGeneratorMode mode);
    CallableParserAndGenerator(Callable &callable, Package *p, CallableParserAndGeneratorMode mode,
                               TypeContext typeContext, CallableWriter &writer, CallableScoper &scoper);
    
    /** Performs the analyziation. */
    void analyze(bool compileDeadCode = false);
    /** Whether self was used in the callable body. */
    bool usedSelfInBody() const { return usedSelf; }
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
    
    /**
     * Safely tries to parse the given token, evaluate the associated command and returns the type of that command.
     * @param token The token to evaluate. Can be @c nullptr which leads to a compiler error.
     * @param expectation The type which is expected to be produced by the following lexical unit. It’s taken into
                          account when parsing numbers etc. Pass @c typeNothingness to indicate no expectations.
     */
    Type parse(const Token &token, Type expectation = typeNothingness);
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
    std::pair<Type, TypeAvailability> parseTypeAsValue(TypeContext tc, SourcePosition p,
                                                       Type expectation = typeNothingness);
    /** Parses an identifier when occurring without context. */
    Type parseIdentifier(const Token &token, Type expectation);
    /** Parses the expression for an if statement. */
    void parseIfExpression(const Token &token);
    /**
     * Parses a function call. This method takes care of parsing all arguments as well as generic arguments and of
     * infering them if necessary.
     * @param type The type for the type context, therefore the type on which this function was called.
     */
    Type parseFunctionCall(Type type, Function *p, const Token &token);
    
    /**
     * Writes a command to access a variable.
     * @param stack The command to access the variable if it is on the stack.
     * @param object The command to access the variable it it is an instance variable.
     */
    void writeCoinForScopesUp(bool inObjectScope, EmojicodeCoin stack, EmojicodeCoin object, SourcePosition p);
    
    void noReturnError(SourcePosition p);
    void noEffectWarning(const Token &warningToken);
    bool typeIsEnumerable(Type type, Type *elementType);
    void flowControlBlock(bool block = true);

    void flowControlReturnEnd(FlowControlReturn &fcr);
    
    void notStaticError(TypeAvailability t, SourcePosition p, const char *name);
    bool isStatic(TypeAvailability t) { return t == TypeAvailability::StaticAndUnavailable
                                                    || t == TypeAvailability::StaticAndAvailabale; }
};

#endif /* CallableParserAndGenerator_hpp */
