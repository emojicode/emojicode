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

extern std::vector<const Token *> stringPool;

class StaticFunctionAnalyzer {
public:
    static void writeAndAnalyzeProcedure(Procedure &procedure, Writer &writer, Type classType, bool inClassContext = false, Initializer *i = nullptr);
    StaticFunctionAnalyzer(Callable &callable, EmojicodeChar ns, Initializer *i, bool inClassContext, Type contextType, Writer &writer);
    
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
    Type contextType;
    
    EmojicodeChar currentNamespace;
    
    /**
     * Safely trys to parse the given token, evaluate the associated command and returns the type of that command.
     * @param token The token to evaluate. Can be @c nullptr which leads to a compiler error.
     */
    Type parse(const Token *token, const Token *parentToken);
    
    /**
     * Same as @c parse. This method however forces the returned type to be a type compatible to @c type.
     * @param token The token to evaluate. Can be @c nullptr which leads to a compiler error.
     */
    Type parse(const Token *token, const Token *parentToken, Type type);
    
    /** Unsafely parses. */
    Type unsafeParseIdentifier(const Token *token);
    
    void noReturnError(const Token *errorToken);
    void noEffectWarning(const Token *warningToken);
    
    /**
     * Checks that the given Procedure can be called from this context.
     */
    void checkAccess(Procedure *p, const Token *token, const char *type);
    /**
     * Parses and validates the arguments for a function.
     * @param calledType The type on which the function is executed. Can be Nothingness.
     */
    void checkArguments(Arguments arguments, Type calledType, const Token *token);
    
    /** 
     * Writes a command to access a variable.
     * @param stack The command to access the variable if it is on the stack.
     * @param object The command to access the variable it it is an instance variable.
     */
    void writeCoinForScopesUp(uint8_t scopesUp, const Token *varName, EmojicodeCoin stack, EmojicodeCoin object);
    
    /** Parses the expression for an if statement. */
    void parseIfExpression(const Token *token);
    
    /** Returns the next variable ID or issues an error if the limit of variables has been exceeded. */
    uint8_t nextVariableID();
    
    void flowControlBlock();
};

#endif /* StaticFunctionAnalyzer_hpp */
