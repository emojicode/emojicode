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
    auto vf = fg->builder().CreateExtractValue(value_->generate(fg), 0);
    return fg->builder().CreateICmpEQ(vf, fg->generator()->optionalNoValue());
}

Value* ASTUnwrap::generate(FunctionCodeGenerator *fg) const {
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

Value* ASTMetaTypeFromInstance::generate(FunctionCodeGenerator *fg) const {
    return fg->getMetaFromObject(value_->generate(fg));
}

}  // namespace EmojicodeCompiler
