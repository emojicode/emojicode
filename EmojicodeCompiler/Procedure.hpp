//
//  Procedure.h
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Procedure_h
#define Procedure_h

#include "utf8.h"
#include "Writer.hpp"

typedef std::vector<Variable> Arguments;

enum AccessLevel {
    PUBLIC, PRIVATE, PROTECTED
};

/** A callable object. */
class Callable {
public:
    Callable(const Token *dToken) : dToken(dToken) {};
    
    /** Parses the arguments for this function. */
    void parseArgumentList(Type ct, EmojicodeChar enamespace);
    /** Parses the return type for this function if there is one specified. */
    void parseReturnType(Type ct, EmojicodeChar theNamespace);
    
    Arguments arguments;
    /** Return type of the method */
    Type returnType = typeNothingness;
    
    /** The first token of this callable. */
    const Token *firstToken;
    
    /** const Token at which this function was defined */
    const Token *dToken;
    
    /** The type of this callable when used as value. */
    virtual Type type() { return typeNothingness; };
};

class Closure: public Callable {
public:
    Closure(const Token *dToken) : Callable(dToken) {};
    Type type();
};

/** Procedures are callables that belong to a class as either method, class method or initialiser. */
class Procedure: public Callable {
public:
    Procedure(EmojicodeChar name, AccessLevel level, bool final, Class *eclass,
              EmojicodeChar theNamespace, const Token *dToken, bool overriding, const Token *documentationToken) :
    Callable(dToken),
    name(name), access(level), eclass(eclass), enamespace(theNamespace), overriding(overriding), documentationToken(documentationToken) {}
    
    /** The procedure name. A Unicode code point for an emoji */
    EmojicodeChar name;
    
    /** Whether the method is native */
    bool native : 1;
    
    bool final : 1;
    bool overriding : 1;
    
    AccessLevel access;
    
    /** Class which defined this Procedure */
    Class *eclass;
    
    const Token *documentationToken;
    
    uint16_t vti;
    
    EmojicodeChar enamespace;
    
    template <typename T>
    void duplicateDeclarationCheck(std::map<EmojicodeChar, T> dict) {
        if (dict.count(name)) {
            ecCharToCharStack(name, nameString);
            compilerError(dToken, "%s %s is declared twice.", on, nameString);
        }
    }
    /**
     * Check whether this procedure is breaking promises.
     */
    void checkPromises(Procedure *superProcedure, const char *on, Type contextType);
    
    void checkOverride(Procedure *superProcedure);
    
    virtual Type type();
private:
    const char *on;
};

class Method: public Procedure {
    using Procedure::Procedure;
    
    const char *on = "Method";
};

class ClassMethod: public Procedure {
    using Procedure::Procedure;
    
    const char *on = "Class Method";
};

class Initializer: public Procedure {
public:
    Initializer(EmojicodeChar name, AccessLevel level, bool final, Class *eclass, EmojicodeChar theNamespace,
                const Token *dToken, bool overriding, const Token *documentationToken, bool r, bool crn) : Procedure(name, level, final, eclass, theNamespace, dToken, overriding, documentationToken), required(r), canReturnNothingness(crn) {}
    
    bool required : 1;
    bool canReturnNothingness : 1;
    const char *on = "Initializer";
    
    Type type();
};

#endif /* Procedure_h */
