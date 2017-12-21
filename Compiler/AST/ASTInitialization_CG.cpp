//
//  _CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Generation/FunctionCodeGenerator.hpp"
#include "ASTInitialization.hpp"
#include "ASTTypeExpr.hpp"
#include "Functions/CallType.h"
#include "Generation/CallCodeGenerator.hpp"
#include "Types/Class.hpp"
#include "Types/Enum.hpp"

namespace EmojicodeCompiler {

Value* ASTInitialization::generate(FunctionCodeGenerator *fg) const {
    switch (initType_) {
        case InitType::Class:
            return generateClassInit(fg);
        case InitType::Enum:
            return llvm::ConstantInt::get(llvm::Type::getInt64Ty(fg->generator()->context()),
                                          typeExpr_->expressionType().eenum()->getValueFor(name_).second);
        case InitType::ValueType:
            assert(vtDestination_ != nullptr);
            InitializationCallCodeGenerator(fg, CallType::StaticDispatch)
            .generate(vtDestination_, typeExpr_->expressionType(), args_, name_);
            return nullptr;
        case InitType::MemoryAllocation:
            return generateMemoryAllocation(fg);
    }
    throw std::logic_error("Unexpected init type");
}

Value* ASTInitialization::generateClassInit(FunctionCodeGenerator *fg) const {
    if (typeExpr_->availability() == TypeAvailability::StaticAndAvailabale) {
        auto type = llvm::dyn_cast<llvm::PointerType>(fg->typeHelper().llvmTypeFor(typeExpr_->expressionType()));

        auto obj = fg->alloc(type);

        fg->builder().CreateStore(typeExpr_->expressionType().eclass()->classMeta(), fg->getObjectMetaPtr(obj));

        auto callGen = InitializationCallCodeGenerator(fg, CallType::StaticDispatch);
        return callGen.generate(obj, typeExpr_->expressionType(), args_, name_);
    }
    // TODO: class table lookup
    throw std::logic_error("Unimplemented");
}

Value* ASTInitialization::generateMemoryAllocation(FunctionCodeGenerator *fg) const {
    return fg->builder().CreateCall(fg->generator()->runTimeNew(), args_.arguments()[0]->generate(fg), "alloc");
}

}  // namespace EmojicodeCompiler
