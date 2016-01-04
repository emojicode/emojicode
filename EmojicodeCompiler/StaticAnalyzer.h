//
//  StaticAnalyzer.h
//  Emojicode
//
//  Created by Theo Weidmann on 05.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef __Emojicode__StaticAnalyzer__
#define __Emojicode__StaticAnalyzer__

#include "EmojicodeCompiler.h"

/**
 * The static analyzer analyses all method and initializer bodies.
 */

/** 
 * Analyzes all eclass
 */
void analyzeClassesAndWrite(FILE *out);

struct StaticInformation {
    /** This points to the Initializer if we are analyzing an initializer. Set to @c NULL in an initializer. */
    Initializer *initializer;
    
    /** The expected return type. The value is undefined if @c initializer is set. */
    Type returnType;
    
    int flowControlDepth;
    
    /** Counts the local varaibles and provides the next ID for a variable. */
    uint8_t variableCount;
    
    /** Whether the statment has an effect. */
    bool effect : 1;
    
    /** Whether the procedure in compilation returned. */
    bool returned : 1;
    
    /** Whether 🐕 or an instance variable has been acessed. */
    bool usedSelf : 1;
    
    /** Set to true when compiling a eclass method. */
    bool inClassContext : 1;
    
    /** Whether the superinitializer has been called. */
    bool calledSuper : 1;
    
    /** The eclass type of the eclass which is compiled. */
    Type classTypeContext;
    
    EmojicodeChar currentNamespace;
};

/** Parses a token and returns the type expected at runtime */
Type typeParse(Token *token, StaticInformation *);

void analyzeFunctionBody(Token *firstToken, Arguments arguments, StaticInformation *SI);

void analyzeFunctionBodyFull(Token *firstToken, Arguments arguments, StaticInformation *SI, bool compileDeadCode, Scope *copyScope);

#endif /* defined(__Emojicode__StaticAnalyzer__) */
