//
//  Function.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Function.hpp"
#include <map>
#include <stdexcept>
#include <numeric>
#include "Lexer.hpp"
#include "CompilerError.hpp"
#include "EmojicodeCompiler.hpp"
#include "TypeContext.hpp"
#include "VTIProvider.hpp"

bool Function::foundStart = false;
Function *Function::start;
int Function::nextVti_ = 0;
std::queue<Function*> Function::compilationQueue;

void Function::setLinkingTableIndex(int index) {
    linkingTableIndex_ = index;
}

void Function::enforcePromises(Function *super, TypeContext typeContext,
                               std::experimental::optional<TypeContext> protocol) {
    try {
        if (super->final()) {
            throw CompilerError(position(), "%s’s implementation of %s was marked 🔏.",
                                super->owningType().toString(typeContext, true).c_str(), name().utf8().c_str());
        }
        if (this->accessLevel() != super->accessLevel()) {
            throw CompilerError(position(), "Access level of %s’s implementation of %s doesn‘t match.",
                                super->owningType().toString(typeContext, true).c_str(), name().utf8().c_str());
        }

        auto superReturnType = protocol ? super->returnType.resolveOn(*protocol, false) : super->returnType;
        if (!returnType.compatibleTo(superReturnType, typeContext)) {
            auto supername = superReturnType.toString(typeContext, true);
            auto thisname = returnType.toString(typeContext, true);
            throw CompilerError(position(), "Return type %s of %s is not compatible to the return type defined in %s.",
                                returnType.toString(typeContext, true).c_str(), name().utf8().c_str(),
                                super->owningType().toString(typeContext, true).c_str());
        }
        if (superReturnType.storageType() == StorageType::Box) {
            returnType.forceBox();
        }
        if (returnType.storageType() != superReturnType.storageType()) {
            throw CompilerError(position(), "Return type is too deviating in memory from super return type. "
                                "Considering Promise broken.");
        }

        if (super->arguments.size() != arguments.size()) {
            throw CompilerError(position(), "Argument count does not match.");
        }
        for (size_t i = 0; i < super->arguments.size(); i++) {
            // More general arguments are OK
            auto superArgumentType = protocol ? super->arguments[i].type.resolveOn(*protocol, false) :
            super->arguments[i].type;
            if (!superArgumentType.compatibleTo(arguments[i].type, typeContext)) {
                auto supertype = superArgumentType.toString(typeContext, true);
                auto thisname = arguments[i].type.toString(typeContext, true);
                throw CompilerError(position(),
                                    "Type %s of argument %d is not compatible with its %s argument type %s.",
                                    thisname.c_str(), i + 1, supertype.c_str());
            }
            if (arguments[i].type.storageType() != superArgumentType.storageType()) {
                throw CompilerError(position(), "Argument %d is too deviating in memory from super argument. "
                                    "Considering Promise broken.", i + 1);
            }
        }
    }
    catch (CompilerError &ce) {
        printError(ce);
    }
}

bool Function::checkOverride(Function *superFunction) const {
    if (overriding()) {
        if (!superFunction || superFunction->accessLevel() == AccessLevel::Private) {
            throw CompilerError(position(), "%s was declared ✒️ but does not override anything.",
                                name().utf8().c_str());
        }
        return true;
    }

    if (superFunction && superFunction->accessLevel() != AccessLevel::Private) {
        throw CompilerError(position(), "If you want to override %s add ✒️.", name().utf8().c_str());
    }
    return false;
}

void Function::deprecatedWarning(SourcePosition p) const {
    if (deprecated()) {
        if (documentation().size() > 0) {
            compilerWarning(p,
                            "%s is deprecated. Please refer to the documentation for further information:\n%s",
                            name().utf8().c_str(), documentation().utf8().c_str());
        }
        else {
            compilerWarning(p, "%s is deprecated.", name().utf8().c_str());
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
    else {
        fullSize_ = std::accumulate(arguments.begin(), arguments.end(), 0, [](int a, Argument b) {
            return a + b.type.size();
        });
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
    Type t = Type::callableIncomplete();
    t.genericArguments.push_back(Type(owningType().eclass(), canReturnNothingness));
    for (size_t i = 0; i < arguments.size(); i++) {
        t.genericArguments.push_back(arguments[i].type);
    }
    return t;
}
