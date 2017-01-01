//
//  Function.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include <map>
#include <stdexcept>
#include "utf8.h"
#include "Function.hpp"
#include "Lexer.hpp"
#include "CompilerErrorException.hpp"
#include "EmojicodeCompiler.hpp"
#include "TypeContext.hpp"
#include "VTIProvider.hpp"

bool Function::foundStart = false;
Function *Function::start;
int Function::nextVti_ = 0;
std::queue<Function*> Function::compilationQueue;

//MARK: Function

void Function::setLinkingTableIndex(int index) {
    linkingTableIndex_ = index;
}

void Function::checkReturnPromise(Type returnThis, Type returnSuper, EmojicodeString name, SourcePosition position,
                                   const char *on, Type contextType) {
    if (!returnThis.compatibleTo(returnSuper, contextType)) {
        auto supername = returnSuper.toString(contextType, true);
        auto thisname = returnThis.toString(contextType, true);
        throw CompilerErrorException(position,
                                     "Return type %s of %s is not compatible with the return type %s of its %s.",
                                     thisname.c_str(), name.utf8().c_str(), supername.c_str(), on);
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

void Function::checkArgumentCount(size_t thisCount, size_t superCount, EmojicodeString name, SourcePosition position,
                                   const char *on, Type contextType) {
    if (superCount != thisCount) {
        throw CompilerErrorException(position, "%s expects %s arguments than its %s.", name.utf8().c_str(),
                                     superCount < thisCount ? "more" : "less", on);
    }
}

void Function::checkPromises(Function *superFunction, const char *on, Type contextType) {
    try {
        if (superFunction->final()) {
            throw CompilerErrorException(this->position(), "%s of %s was marked 🔏.", on, name().utf8().c_str());
        }
        if (this->accessLevel() != superFunction->accessLevel()) {
            throw CompilerErrorException(this->position(), "The access level of %s and its %s don’t match.",
                                         name().utf8().c_str(), on);
        }
        checkReturnPromise(this->returnType, superFunction->returnType, this->name(), this->position(), on, contextType);
        checkArgumentCount(this->arguments.size(), superFunction->arguments.size(), this->name(),
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

bool Function::checkOverride(Function *superFunction) {
    if (overriding()) {
        if (!superFunction || superFunction->accessLevel() == AccessLevel::Private) {
            throw CompilerErrorException(position(), "%s was declared ✒️ but does not override anything.",
                                         name().utf8().c_str());
        }
        return true;
    }
    
    if (superFunction && superFunction->accessLevel() != AccessLevel::Private) {
        throw CompilerErrorException(position(), "If you want to override %s add ✒️.", name().utf8().c_str());
    }
    return false;
}

void Function::deprecatedWarning(const Token &callToken) {
    if (deprecated()) {
        if (documentation().size() > 0) {
            compilerWarning(callToken,
                            "%s is deprecated. Please refer to the documentation for further information:\n%s",
                            name().utf8().c_str(), documentation().utf8().c_str());
        }
        else {
            compilerWarning(callToken, "%s is deprecated.", name().utf8().c_str());
        }
    }
}

void Function::assignVti() {
    if (!assigned()) {
        setVti(vtiProvider_->next());
    }
}

void Function::markUsed() {
    if (used_) {
        return;
    }
    used_ = true;
    if (vtiProvider_) {
        vtiProvider_->used();
    }
    if (!isNative()) {
        Function::compilationQueue.push(this);
    }
    for (Function *function : overriders_) {
        function->markUsed();
    }
}

int Function::vtiForUse() {
    if (!assigned()) {
        setVti(vtiProvider_->next());
    }
    if (!used_) {
        markUsed();
    }
    return vti_;
}

int Function::getVti() const {
    if (!assigned()) {
        throw std::logic_error("Getting VTI from unassinged function.");
    }
    return vti_;
}

void Function::setVti(int vti) {
    if (assigned()) {
        throw std::logic_error("You cannot reassign the VTI.");
    }
    vti_ = vti;
}

bool Function::assigned() const {
    return vti_ >= 0;
}

void Function::setVtiProvider(VTIProvider *provider) {
    if (vtiProvider_) {
        throw std::logic_error("You cannot reassign the VTI provider.");
    }
    vtiProvider_ = provider;
}

Type Initializer::type() const {
    Type t = Type(TypeContent::Callable, false);
    t.genericArguments.push_back(Type(owningType().eclass(), canReturnNothingness));
    for (size_t i = 0; i < arguments.size(); i++) {
        t.genericArguments.push_back(arguments[i].type);
    }
    return t;
}

bool Function::typeMethod() const {
    return compilationMode_ == CallableParserAndGeneratorMode::Function ||
            compilationMode_ == CallableParserAndGeneratorMode::ClassMethod;
}
