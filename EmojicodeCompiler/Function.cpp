//
//  Function.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include <map>
#include "utf8.h"
#include "Function.hpp"
#include "Lexer.hpp"
#include "CompilerErrorException.hpp"
#include "EmojicodeCompiler.hpp"
#include "TypeContext.hpp"

//MARK: Closure

Type Closure::type() {
    Type t(TypeContent::Callable, false);
    t.arguments = (uint8_t)arguments.size();
    
    t.genericArguments.push_back(returnType);
    for (int i = 0; i < arguments.size(); i++) {
        t.genericArguments.push_back(arguments[i].type);
    }
    return t;
}

bool Function::foundStart = false;
Function *Function::start;

//MARK: Function

void Function::checkReturnPromise(Type returnThis, Type returnSuper, EmojicodeChar name, SourcePosition position,
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

void Function::checkArgument(Type thisArgument, Type superArgument, int index, SourcePosition position,
                              const char *on, Type contextType) {
    // Other way, because the function may take more general arguments
    if (!superArgument.compatibleTo(thisArgument, contextType)) {
        auto supertype = superArgument.toString(contextType, true);
        auto thisname = thisArgument.toString(contextType, true);
        throw CompilerErrorException(position,
                      "Type %s of argument %d is not compatible with its %s argument type %s.",
                      thisname.c_str(), index + 1, on, supertype.c_str());
    }
}

void Function::checkArgumentCount(size_t thisCount, size_t superCount, EmojicodeChar name, SourcePosition position,
                                   const char *on, Type contextType) {
    if (superCount != thisCount) {
        ecCharToCharStack(name, mn);
        throw CompilerErrorException(position, "%s expects %s arguments than its %s.", mn,
                      (superCount < thisCount) ? "more" : "less", on);
    }
}

void Function::checkPromises(Function *superFunction, const char *on, Type contextType) {
    try {
        if (superFunction->final) {
            ecCharToCharStack(this->name, mn);
            throw CompilerErrorException(this->position(), "%s of %s was marked 🔏.", on, mn);
        }
        checkReturnPromise(this->returnType, superFunction->returnType, this->name, this->position(), on, contextType);
        checkArgumentCount(this->arguments.size(), superFunction->arguments.size(), this->name,
                           this->position(), on, contextType);

        for (int i = 0; i < superFunction->arguments.size(); i++) {
            checkArgument(this->arguments[i].type, superFunction->arguments[i].type, i,
                          this->position(), on, contextType);
        }
    }
    catch (CompilerErrorException &ce) {
        printError(ce);
    }
}

void Function::checkOverride(Function *superFunction) {
    if (overriding && !superFunction) {
        ecCharToCharStack(name, mn);
        throw CompilerErrorException(position(), "%s was declared ✒️ but does not override anything.", mn);
    }
    else if (!overriding && superFunction) {
        ecCharToCharStack(name, mn);
        throw CompilerErrorException(position(), "If you want to override %s add ✒️.", mn);
    }
}

void Function::deprecatedWarning(const Token &callToken) {
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

void Function::setVti(int vti) {
    if (vti >= 65536) {
        throw CompilerErrorException(position(), "You exceeded the limit of 65,536 %s in a class.", on);
    }
    vti_ = vti;
}

Type Function::type() {
    Type t = Type(TypeContent::Callable, false);
    t.arguments = (uint8_t)arguments.size();
    
    t.genericArguments.push_back(returnType);
    for (size_t i = 0; i < arguments.size(); i++) {
        t.genericArguments.push_back(arguments[i].type);
    }
    return t;
}

Type Initializer::type() {
    Type t = Type(TypeContent::Callable, false);
    t.arguments = (uint8_t)arguments.size();
    
    t.genericArguments.push_back(Type(owningType.eclass(), canReturnNothingness));
    for (size_t i = 0; i < arguments.size(); i++) {
        t.genericArguments.push_back(arguments[i].type);
    }
    return t;
}
