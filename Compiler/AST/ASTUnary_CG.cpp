//
//  ASTUnary_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTUnary.hpp"

namespace EmojicodeCompiler {

Value* ASTIsError::generate(FunctionCodeGenerator *fg) const {
    if (value_->expressionType().storageType() == StorageType::Box) {
        return fg->getHasNoValueBox(value_->generate(fg));
    }
    return fg->getIsError(value_->generate(fg));
}

Value* ASTUnwrap::generate(FunctionCodeGenerator *fg) const {
    if (error_) {
        return generateErrorUnwrap(fg);
    }

    auto optional = value_->generate(fg);
    auto isBox = value_->expressionType().storageType() == StorageType::Box;
    auto hasNoValue = isBox ? fg->getHasNoValueBox(optional) : fg->getHasNoValue(optional);

    fg->createIfElseBranchCond(hasNoValue, [this, fg]() {
        std::stringstream str;
        str << "Unwrapped an optional that contained no value.";
        str << " (" << position().file << ":" << position().line << ":" << position().character << ")";
        auto string = fg->builder().CreateGlobalStringPtr(str.str());
        fg->builder().CreateCall(fg->generator()->declarator().panic(), string);
        fg->builder().CreateUnreachable();
        return false;
    }, []() { return true; });
    if (isBox) {
        return optional;
    }
    return fg->builder().CreateExtractValue(optional, 1);
}

Value* ASTUnwrap::generateErrorUnwrap(FunctionCodeGenerator *fg) const {
    auto error = value_->generate(fg);
    auto isBox = value_->expressionType().storageType() == StorageType::Box;
    auto hasNoValue = isBox ? fg->getHasNoValueBox(error) : fg->getIsError(error);

    fg->createIfElseBranchCond(hasNoValue, [this, fg]() {
        std::stringstream str;
        str << "Unwrapped an error that contained an error.";
        str << " (" << position().file << ":" << position().line << ":" << position().character << ")";
        auto string = fg->builder().CreateGlobalStringPtr(str.str());
        fg->builder().CreateCall(fg->generator()->declarator().panic(), string);
        fg->builder().CreateUnreachable();
        return false;
    }, []() { return true; });
    if (isBox) {
        return error;
    }
    return fg->builder().CreateExtractValue(error, 1);
}
    
}  // namespace EmojicodeCompiler
