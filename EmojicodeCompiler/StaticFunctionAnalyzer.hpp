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
#include "Procedure.hpp"
#include "Writer.hpp"
#include "CompilerScope.hpp"
#include "AbstractParser.hpp"

struct FlowControlReturn {
    int branches = 0;
    int branchReturns = 0;
    
    bool returned() { return branches == branchReturns; }
};

class StaticFunctionAnalyzer : AbstractParser {
public:
    static void writeAndAnalyzeProcedure(Procedure *procedure, Writer &writer, Type classType, Scoper &scoper,
                                         bool inClassContext = false, Initializer *i = nullptr);
    StaticFunctionAnalyzer(Callable &callable, Package *p, Initializer *i, bool inClassContext,
                           TypeContext typeContext, Writer &writer, Scoper &scoper);
    
    /** Performs the analyziation. */
    void analyze(bool compileDeadCode = false, Scope *copyScope = nullptr);
    /** Whether self was used in the callable body. */
    bool usedSelfInBody() { return usedSelf; };
    /** The number of local variables created in the function. */
    uint8_t localVariableCount() { return variableCount; };
private:
    /** The callable which is processed. */
    Callable &callable;
    /** The writer used for writing the byte code. */
    Writer &writer;
    
    Scoper &scoper;
    
    /** This points to the Initializer if we are analyzing an initializer. Set to @c nullptr in an initializer. */
    Initializer *initializer = nullptr;
    /** The flow control depth. */
    int flowControlDepth = 0;
    /** Counts the local varaibles and provides the next ID for a variable. */
    uint8_t variableCount = 0;
    /** Whether the statment has an effect. */
    bool effect = false;
    /** Whether the procedure in compilation returned. */
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
     */
    Type parse(const Token &token);
    
    /**
     * Same as @c parse. This method however forces the returned type to be a type compatible to @c type.
     * @param token The token to evaluate. Can be @c nullptr which leads to a compiler error.
     */
    Type parse(const Token &token, const Token &parentToken, Type type);
    
    Type parseIdentifier(const Token &token);
    /** Parses the expression for an if statement. */
    void parseIfExpression(const Token &token);
    
    void noReturnError(SourcePosition p);
    void noEffectWarning(const Token &warningToken);
    
    /**
     * Checks that the given Procedure can be called from this context.
     */
    void checkAccess(Procedure *p, const Token &token, const char *type);
    
    std::vector<Type> checkGenericArguments(Procedure *p, const Token &token);
    
    /**
     * Parses and validates the arguments for a function.
     * @param calledType The type on which the function is executed. Can be Nothingness.
     */
    void checkArguments(std::vector<Variable> arguments, TypeContext calledType, const Token &token);
    
    bool typeIsEnumerable(Type type, Type *elementType);
    
    /** 
     * Writes a command to access a variable.
     * @param stack The command to access the variable if it is on the stack.
     * @param object The command to access the variable it it is an instance variable.
     */
    void writeCoinForScopesUp(uint8_t scopesUp, const Token &varName, EmojicodeCoin stack, EmojicodeCoin object);
    
    /** Returns the next variable ID or issues an error if the limit of variables has been exceeded. */
    uint8_t nextVariableID();
    
    void flowControlBlock();
    
    void flowControlReturnEnd(FlowControlReturn &fcr);
};

#endif /* StaticFunctionAnalyzer_hpp */
