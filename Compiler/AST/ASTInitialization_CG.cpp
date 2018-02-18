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
        return initObject(fg, args_, name_, typeExpr_->expressionType());
    }
    // TODO: class table lookup
    throw std::logic_error("Unimplemented");
}

Value * ASTInitialization::initObject(FunctionCodeGenerator *fg, const ASTArguments &args, const std::u32string &name,
                                      const Type &type) {
    auto llvmType = llvm::dyn_cast<llvm::PointerType>(fg->typeHelper().llvmTypeFor(type));

    auto obj = fg->alloc(llvmType);

    fg->builder().CreateStore(type.eclass()->classMeta(), fg->getObjectMetaPtr(obj));

    auto callGen = InitializationCallCodeGenerator(fg, CallType::StaticDispatch);
    callGen.generate(obj, type, args, name);
    return obj;
}

Value* ASTInitialization::generateMemoryAllocation(FunctionCodeGenerator *fg) const {
    return fg->builder().CreateCall(fg->generator()->declarator().runTimeNew(),
                                    args_.arguments()[0]->generate(fg), "alloc");
}

}  // namespace EmojicodeCompiler
