//
//  Procedure.h
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Procedure_h
#define Procedure_h

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
    Callable(dToken), name(name),  overriding(overriding), access(level), eclass(eclass), documentationToken(documentationToken), enamespace(theNamespace) {}
    
    /** The procedure name. A Unicode code point for an emoji */
    EmojicodeChar name;
    
    /** Whether the method is native. */
    bool native = false;
    bool final = false;
    bool overriding = false;
    
    AccessLevel access;
    
    /** Class which defined this procedure. This can be @c nullptr if the procedure belongs to a protocol. */
    Class *eclass;
    
    const Token *documentationToken;
    
    uint16_t vti;
    
    /** The namespace in which the procedure was defined. This does not necessarily match the class’s namespace. */
    EmojicodeChar enamespace;
    
    /**
     * Check whether this procedure is breaking promises of @c superProcedure.
     */
    void checkPromises(Procedure *superProcedure, const char *on, Type contextType);
    
    void checkOverride(Procedure *superProcedure);
    
    void parseBody(bool allowNative);
    
    virtual Type type();
    
    const char *on;
};

class Method: public Procedure {
    using Procedure::Procedure;
public:
    const char *on = "Method";
};

class ClassMethod: public Procedure {
    using Procedure::Procedure;
public:
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
