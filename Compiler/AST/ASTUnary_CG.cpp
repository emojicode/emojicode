//
//  ASTUnary_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTUnary.hpp"
#include "Generation/RunTimeHelper.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "Lex/SourceManager.hpp"
#include "Generation/CallCodeGenerator.hpp"
#include "Compiler.hpp"
#include "ASTLiterals.hpp"
#include "Types/TypeContext.hpp"
#include <sstream>

namespace EmojicodeCompiler {

llvm::Value* createExpectFalse(FunctionCodeGenerator *fg, llvm::Value *value) {
    return fg->builder().CreateIntrinsic(llvm::Intrinsic::expect, llvm::Type::getInt1Ty(fg->ctx()),
                                         { value, fg->builder().getInt1(false) });
}

Value* ASTUnwrap::generate(FunctionCodeGenerator *fg) const {
    if (error_) {
        return generateErrorUnwrap(fg);
    }

    auto optional = expr_->generate(fg);
    auto isBox = expr_->expressionType().storageType() == StorageType::Box;
    auto hasNoValue = isBox ? fg->buildHasNoValueBox(optional)
                            : fg->buildOptionalHasNoValue(optional, expr_->expressionType());


    fg->createIfElseBranchCond(createExpectFalse(fg, hasNoValue), [this, fg]() {
        std::stringstream str;
        str << "Unwrapped an optional that contained no value. (" << position().toRuntimeString() << ")";
        auto string = fg->builder().CreateGlobalStringPtr(str.str());
        fg->builder().CreateCall(fg->generator()->runTime().panic(), string);
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
    fg->createIfElseBranchCond(createExpectFalse(fg, isError(fg, errorDest)), [&]() {
        auto string = std::make_shared<ASTCGUTF8Literal>(position().toRuntimeString(), position());
        CallCodeGenerator(fg, CallType::StaticDispatch).generate(fg->builder().CreateLoad(errorDest),
                                                                 Type(fg->compiler()->sError),
                                                                 ASTArguments(position(), { string }),
                                                                 method_, nullptr);
        fg->builder().CreateUnreachable();
        return false;
    }, []() { return true; });
    return value;
}

Value* ASTReraise::generate(FunctionCodeGenerator *fg) const {
    dynamic_cast<ASTCall *>(expr_.get())->setErrorPointer(fg->errorPointer());
    auto value = expr_->generate(fg);
    fg->createIfElseBranchCond(createExpectFalse(fg, isError(fg, fg->errorPointer())), [this, fg]() {
        fg->releaseTemporaryObjects(false, expr_->producesTemporaryObject());
        release(fg);
        fg->buildErrorReturn();
        return false;
    }, []() { return true; });
    return value;
}

}  // namespace EmojicodeCompiler
