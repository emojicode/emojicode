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

typedef std::vector<Variable> Arguments;

class Procedure {
public:
    Procedure(EmojicodeChar name, AccessLevel level, bool final, Class *eclass,
              EmojicodeChar theNamespace, Token *dToken, bool overriding, Token *documentationToken) :
    name(name), access(level), eclass(eclass), enamespace(theNamespace), dToken(dToken), overriding(overriding), documentationToken(documentationToken), returnType(typeNothingness) {}
    
    /** The procedure name. A Unicode code point for an emoji */
    EmojicodeChar name;
    
    Arguments arguments;
    
    /** Whether the method is native */
    bool native : 1;
    
    bool final : 1;
    bool overriding : 1;
    
    AccessLevel access;
    /** Class which defined this method */
    Class *eclass;
    
    /** Token at which this method was defined */
    Token *dToken;
    Token *documentationToken;
    
    uint16_t vti;
    
    /** Return type of the method */
    Type returnType;
    
    Token *firstToken;
    
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
                Token *dToken, bool overriding, Token *documentationToken, bool r, bool crn) : Procedure(name, level, final, eclass, theNamespace, dToken, overriding, documentationToken), required(r), canReturnNothingness(crn) {}
    
    bool required : 1;
    bool canReturnNothingness : 1;
    const char *on = "Initializer";
};

#endif /* Procedure_h */
