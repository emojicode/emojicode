//
//  Function.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Function.hpp"
#include "CompilerError.hpp"
#include "EmojicodeCompiler.hpp"
#include "Generation/VTIProvider.hpp"
#include "Types/TypeContext.hpp"
#include <algorithm>
#include <map>
#include <stdexcept>

namespace EmojicodeCompiler {

bool Function::foundStart = false;
Function *Function::start;
std::queue<Function*> Function::compilationQueue;
std::queue<Function*> Function::analysisQueue;

void Function::setLinkingTableIndex(int index) {
    linkingTableIndex_ = index;
}

bool Function::enforcePromises(Function *super, const TypeContext &typeContext, const Type &superSource,
                               std::experimental::optional<TypeContext> protocol) {
    try {
        if (super->final()) {
            throw CompilerError(position(), "%s’s implementation of %s was marked 🔏.",
                                superSource.toString(typeContext).c_str(), name().utf8().c_str());
        }
        if (this->accessLevel() != super->accessLevel()) {
            throw CompilerError(position(), "Access level of %s’s implementation of %s doesn‘t match.",
                                superSource.toString(typeContext).c_str(), name().utf8().c_str());
        }

        auto superReturnType = protocol ? super->returnType.resolveOn(*protocol, false) : super->returnType;
        if (!returnType.resolveOn(typeContext).compatibleTo(superReturnType, typeContext)) {
            auto supername = superReturnType.toString(typeContext);
            auto thisname = returnType.toString(typeContext);
            throw CompilerError(position(), "Return type %s of %s is not compatible to the return type defined in %s.",
                                returnType.toString(typeContext).c_str(), name().utf8().c_str(),
                                superSource.toString(typeContext).c_str());
        }
        if (superReturnType.storageType() == StorageType::Box && !protocol) {
            returnType.forceBox();
        }
        if (returnType.resolveOn(typeContext).storageType() != superReturnType.storageType()) {
            if (protocol) {
                return false;
            }
            throw CompilerError(position(), "Return and super return are storage incompatible.");
        }

        if (super->arguments.size() != arguments.size()) {
            throw CompilerError(position(), "Argument count does not match.");
        }
        for (size_t i = 0; i < super->arguments.size(); i++) {
            // More general arguments are OK
            auto superArgumentType = protocol ? super->arguments[i].type.resolveOn(*protocol, false) :
            super->arguments[i].type;
            if (!superArgumentType.compatibleTo(arguments[i].type.resolveOn(typeContext), typeContext)) {
                auto supertype = superArgumentType.toString(typeContext);
                auto thisname = arguments[i].type.toString(typeContext);
                throw CompilerError(position(),
                                    "Type %s of argument %d is not compatible with its %s argument type %s.",
                                    thisname.c_str(), i + 1, supertype.c_str());
            }
            if (arguments[i].type.resolveOn(typeContext).storageType() != superArgumentType.storageType()) {
                if (protocol) {
                    return false;
                }
                throw CompilerError(position(), "Argument %d and super argument are storage incompatible.", i + 1);
            }
        }
    }
    catch (CompilerError &ce) {
        printError(ce);
    }
    return true;
}

void Function::deprecatedWarning(const SourcePosition &p) const {
    if (deprecated()) {
        if (!documentation().empty()) {
            compilerWarning(p, "%s is deprecated. Please refer to the documentation for further information:\n%s",
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
        for (Function *function : overriders_) {
            function->assignVti();
        }
    }
}

void Function::setUsed(bool enqueue) {
    if (!used_) {
        used_ = true;
        if (vtiProvider_) {
            vtiProvider_->used();
        }
        if (enqueue) {
            Function::compilationQueue.emplace(this);
        }
        for (Function *function : overriders_) {
            function->setUsed();
        }
    }
}

int Function::vtiForUse() {
    assignVti();
    setUsed();
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
    if (vtiProvider_ != nullptr) {
        throw std::logic_error("You cannot reassign the VTI provider.");
    }
    vtiProvider_ = provider;
}

Type Function::type() const {
    Type t = Type::callableIncomplete();
    t.genericArguments_.reserve(arguments.size() + 1);
    t.genericArguments_.push_back(returnType);
    for (auto &argument : arguments) {
        t.genericArguments_.push_back(argument.type);
    }
    return t;
}

}  // namespace EmojicodeCompiler
