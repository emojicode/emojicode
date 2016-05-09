//
//  Procedure.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include <map>
#include "utf8.h"
#include "Procedure.hpp"
#include "Lexer.hpp"
#include "CompilerErrorException.hpp"
#include "EmojicodeCompiler.hpp"
#include "TypeContext.hpp"

//MARK: Closure

Type Closure::type() {
    Type t(TT_CALLABLE, false);
    t.arguments = (uint8_t)arguments.size();
    
    t.genericArguments.push_back(returnType);
    for (int i = 0; i < arguments.size(); i++) {
        t.genericArguments.push_back(arguments[i].type);
    }
    return t;
}

//MARK: Procedure

void Procedure::checkReturnPromise(Type returnThis, Type returnSuper, EmojicodeChar name, SourcePosition position,
                                   const char *on, Type contextType) {
    if (!returnThis.compatibleTo(returnSuper, contextType)) {
        // The return type may be defined more precisely
        ecCharToCharStack(name, mn);
        auto supername = returnSuper.toString(contextType, true);
        auto thisname = returnThis.toString(contextType, true);
        throw CompilerErrorException(position, "Return type %s of %s is not compatible with the return type %s of its %s.",
                      thisname.c_str(), mn, supername.c_str(), on);
    }
}

void Procedure::checkArgument(Type thisArgument, Type superArgument, int index, SourcePosition position,
                              const char *on, Type contextType) {
    // Other way, because the procedure may take more general arguments
    if (!superArgument.compatibleTo(thisArgument, contextType)) {
        auto supertype = superArgument.toString(contextType, true);
        auto thisname = thisArgument.toString(contextType, true);
        throw CompilerErrorException(position,
                      "Type %s of argument %d is not compatible with its %s argument type %s.",
                      thisname.c_str(), index + 1, on, supertype.c_str());
    }
}

void Procedure::checkArgumentCount(size_t thisCount, size_t superCount, EmojicodeChar name, SourcePosition position,
                                   const char *on, Type contextType) {
    if (superCount != thisCount) {
        ecCharToCharStack(name, mn);
        throw CompilerErrorException(position, "%s expects %s arguments than its %s.", mn,
                      (superCount < thisCount) ? "more" : "less", on);
    }
}

void Procedure::checkPromises(Procedure *superProcedure, const char *on, Type contextType) {
    try {
        if (superProcedure->final) {
            ecCharToCharStack(this->name, mn);
            throw CompilerErrorException(this->position(), "%s of %s was marked 🔏.", on, mn);
        }
        checkReturnPromise(this->returnType, superProcedure->returnType, this->name, this->position(), on, contextType);
        checkArgumentCount(this->arguments.size(), superProcedure->arguments.size(), this->name,
                           this->position(), on, contextType);

        for (int i = 0; i < superProcedure->arguments.size(); i++) {
            checkArgument(this->arguments[i].type, superProcedure->arguments[i].type, i,
                          this->position(), on, contextType);
        }
    }
    catch (CompilerErrorException &ce) {
        printError(ce);
    }
}

void Procedure::checkOverride(Procedure *superProcedure) {
    if (overriding && !superProcedure) {
        ecCharToCharStack(name, mn);
        throw CompilerErrorException(position(), "%s was declared ✒️ but does not override anything.", mn);
    }
    else if (!overriding && superProcedure) {
        ecCharToCharStack(name, mn);
        throw CompilerErrorException(position(), "If you want to override %s add ✒️.", mn);
    }
}

void Procedure::deprecatedWarning(const Token &callToken) {
    if (deprecated) {
        ecCharToCharStack(name, mn);
        if (documentationToken.size() > 0) {
            char *documentation = documentationToken.utf8CString();
            compilerWarning(callToken,
                            "%s is deprecated. Please refer to the documentation for further information:\n%s",
                            mn, documentation);
            delete [] documentation;
        }
        else {
            compilerWarning(callToken, "%s is deprecated.", mn);
        }
    }
}

Type Procedure::type() {
    Type t = Type(TT_CALLABLE, false);
    t.arguments = (uint8_t)arguments.size();
    
    t.genericArguments.push_back(returnType);
    for (size_t i = 0; i < arguments.size(); i++) {
        t.genericArguments.push_back(arguments[i].type);
    }
    return t;
}

Type Initializer::type() {
    Type t = Type(TT_CALLABLE, false);
    t.arguments = (uint8_t)arguments.size();
    
    t.genericArguments.push_back(Type(eclass, canReturnNothingness));
    for (size_t i = 0; i < arguments.size(); i++) {
        t.genericArguments.push_back(arguments[i].type);
    }
    return t;
}
