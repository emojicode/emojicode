//
//  Function.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Function.hpp"
#include "Compiler.hpp"
#include "CompilerError.hpp"
#include "EmojicodeCompiler.hpp"
#include "Types/TypeContext.hpp"
#include <llvm/IR/Function.h>
#include <algorithm>
#include <stdexcept>

namespace EmojicodeCompiler {

bool Function::enforcePromises(Function *super, const TypeContext &typeContext, const Type &superSource,
                               std::experimental::optional<TypeContext> protocol) {
    try {
        if (super->final()) {
            throw CompilerError(position(), superSource.toString(typeContext), "’s implementation of ", utf8(name()),
                                " was marked 🔏.");
        }
        if (this->accessLevel() != super->accessLevel()) {
            throw CompilerError(position(), "Access level of ", superSource.toString(typeContext),
                                "’s implementation of, ", utf8(name()), ", doesn‘t match.");
        }

        auto superReturnType = protocol ? super->returnType.resolveOn(*protocol) : super->returnType;
        if (!returnType.resolveOn(typeContext).compatibleTo(superReturnType, typeContext)) {
            auto supername = superReturnType.toString(typeContext);
            auto thisname = returnType.toString(typeContext);
            throw CompilerError(position(), "Return type ", returnType.toString(typeContext), " of ", utf8(name()),
                                " is not compatible to the return type defined in ", superSource.toString(typeContext));
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
            auto superArgumentType = protocol ? super->arguments[i].type.resolveOn(*protocol) :
            super->arguments[i].type;
            if (!superArgumentType.compatibleTo(arguments[i].type.resolveOn(typeContext), typeContext)) {
                auto supertype = superArgumentType.toString(typeContext);
                auto thisname = arguments[i].type.toString(typeContext);
                throw CompilerError(position(), "Type ", thisname, " of argument ", i + 1,
                                    " is not compatible with its ", thisname, " argument type ", supertype, ".");
            }
            if (arguments[i].type.resolveOn(typeContext).storageType() != superArgumentType.storageType()) {
                if (protocol) {
                    return false;
                }
                throw CompilerError(position(), "Argument ", i + 1, " and super argument are storage incompatible.");
            }
        }
    }
    catch (CompilerError &ce) {
        package_->compiler()->error(ce);
    }
    return true;
}

void Function::deprecatedWarning(const SourcePosition &p) const {
    if (deprecated()) {
        if (!documentation().empty()) {
            package_->compiler()->warn(p, utf8(name()), " is deprecated. Please refer to the "\
                                  "documentation for further information: ", utf8(documentation()));
        }
        else {
            package_->compiler()->warn(p, utf8(name()), " is deprecated.");
        }
    }
}

int Function::vti() const {
    return vti_;
}

void Function::setVti(int vti) {
    vti_ = vti;
}

llvm::FunctionType* Function::llvmFunctionType() const {
    return llvmFunction()->getFunctionType();
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
