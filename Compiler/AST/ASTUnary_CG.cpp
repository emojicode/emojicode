//
//  ASTUnary_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTUnary.hpp"
#include "Generation/Declarator.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "Lex/SourceManager.hpp"
#include "Generation/CallCodeGenerator.hpp"
#include "Compiler.hpp"
#include "ASTLiterals.hpp"
#include "Types/Class.hpp"
#include <llvm/Support/raw_ostream.h>
#include <sstream>

namespace EmojicodeCompiler {

Value* ASTUnwrap::generate(FunctionCodeGenerator *fg) const {
    if (error_) {
        return generateErrorUnwrap(fg);
    }

    auto optional = expr_->generate(fg);
    auto isBox = expr_->expressionType().storageType() == StorageType::Box;
    auto hasNoValue = isBox ? fg->buildHasNoValueBox(optional)
                            : fg->buildOptionalHasNoValue(optional, expr_->expressionType());

    fg->createIfElseBranchCond(hasNoValue, [this, fg]() {
        std::stringstream str;
        str << "Unwrapped an optional that contained no value. (" << position().toRuntimeString() << ")";
        auto string = fg->builder().CreateGlobalStringPtr(str.str());
        fg->builder().CreateCall(fg->generator()->declarator().panic(), string);
        fg->builder().CreateUnreachable();
        return false;
    }, []() { return true; });
    if (isBox) {
        return optional;
    }
    return fg->buildGetOptionalValue(optional, expr_->expressionType());
}

Value* ASTUnwrap::generateErrorUnwrap(FunctionCodeGenerator *fg) const {
    auto errorDest = prepareErrorDestination(fg, expr_.get());
    auto value = expr_->generate(fg);
    fg->createIfElseBranchCond(isError(fg, errorDest), [this, fg, errorDest]() {
        auto error = fg->compiler()->sError;
        auto string = std::make_shared<ASTCGUTF8Literal>(position().toRuntimeString(), position());
        CallCodeGenerator(fg, CallType::StaticDispatch).generate(fg->builder().CreateLoad(errorDest),
                                                                 Type(error), ASTArguments(position(), { string }),
                                                                 error->lookupMethod(U"🤯", Mood::Imperative), nullptr);
        fg->builder().CreateUnreachable();
        return false;
    }, []() { return true; });
    return value;
}

Value* ASTReraise::generate(FunctionCodeGenerator *fg) const {
    dynamic_cast<ASTCall *>(expr_.get())->setErrorPointer(fg->errorPointer());
    auto value = expr_->generate(fg);
    fg->createIfElseBranchCond(isError(fg, fg->errorPointer()), [fg]() {
        // TODO: destroy
        fg->buildErrorReturn();
        return false;
    }, []() { return true; });
    return value;
}

}  // namespace EmojicodeCompiler
