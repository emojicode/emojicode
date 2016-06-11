//
//  StaticFunctionAnalyzer.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef StaticFunctionAnalyzer_hpp
#define StaticFunctionAnalyzer_hpp

#include "EmojicodeCompiler.hpp"
#include "Function.hpp"
#include "Writer.hpp"
#include "CallableScoper.hpp"
#include "AbstractParser.hpp"
#include "TypeContext.hpp"

struct FlowControlReturn {
    int branches = 0;
    int branchReturns = 0;
    
    bool returned() { return branches == branchReturns; }
};

class StaticFunctionAnalyzer : AbstractParser {
public:
    static void writeAndAnalyzeFunction(Function *function, Writer &writer, Type classType, CallableScoper &scoper,
                                         bool inClassContext = false, Initializer *i = nullptr);
    StaticFunctionAnalyzer(Callable &callable, Package *p, Initializer *i, bool inClassContext,
                           TypeContext typeContext, Writer &writer, CallableScoper &scoper);
    
    /** Performs the analyziation. */
    void analyze(bool compileDeadCode = false);
    /** Whether self was used in the callable body. */
    bool usedSelfInBody() { return usedSelf; };
private:
    /** The callable which is processed. */
    Callable &callable;
    /** The writer used for writing the byte code. */
    Writer &writer;
    
    CallableScoper &scoper;
    
    /** This points to the Initializer if we are analyzing an initializer. Set to @c nullptr in an initializer. */
    Initializer *initializer = nullptr;
    /** The flow control depth. */
    int flowControlDepth = 0;
    /** Whether the statment has an effect. */
    bool effect = false;
    /** Whether the function in compilation returned. */
    bool returned = false;
    /** Whether the self reference or an instance variable has been acessed. */
    bool usedSelf = false;
    /** Set to true when compiling a class method. */
    bool inClassContext = false;
    /** Whether the superinitializer has been called. */
    bool calledSuper = false;
    /** The class type of the eclass which is compiled. */
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
     * @param token The token to evaluate. Can be @c nullptr which leads to a compiler error.
     */
    Type parse(const Token &token, const Token &parentToken, Type type, std::vector<CommonTypeFinder>* = nullptr);
    /** Parses an identifier when occurring without context. */
    Type parseIdentifier(const Token &token, Type expectation);
    /** Parses the expression for an if statement. */
    void parseIfExpression(const Token &token);
    
    void noReturnError(SourcePosition p);
    void noEffectWarning(const Token &warningToken);
    
    Type parseFunctionCall(Type type, Function *p, const Token &token);

    bool typeIsEnumerable(Type type, Type *elementType);
    
    /** 
     * Writes a command to access a variable.
     * @param stack The command to access the variable if it is on the stack.
     * @param object The command to access the variable it it is an instance variable.
     */
    void writeCoinForScopesUp(bool inObjectScope, EmojicodeCoin stack, EmojicodeCoin object, SourcePosition p);
    
    void flowControlBlock(bool block = true);
    
    void flowControlReturnEnd(FlowControlReturn &fcr);
};

#endif /* StaticFunctionAnalyzer_hpp */
