//
//  _CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "../Generation/FnCodeGenerator.hpp"
#include "../Functions/CallType.h"
#include "../Types/Enum.hpp"
#include "ASTTypeExpr.hpp"
#include "../Generation/CallCodeGenerator.hpp"
#include "ASTInitialization.hpp"
#include "ASTProxyExpr.hpp"

namespace EmojicodeCompiler {

Value* ASTInitialization::generateExpr(FnCodeGenerator *fncg) const {
    switch (initType_) {
        case InitType::Class:
            return generateClassInit(fncg);
        case InitType::Enum:
            return llvm::ConstantInt::get(llvm::Type::getInt64Ty(fncg->generator()->context()),
                                          typeExpr_->expressionType().eenum()->getValueFor(name_).second);
            break;
        case InitType::ValueType:
            VTInitializationCallCodeGenerator(fncg).generate(vtDestination_, typeExpr_->expressionType(), args_, name_);
    }
    return nullptr;
}

Value* ASTInitialization::generateClassInit(FnCodeGenerator *fncg) const {
    if (typeExpr_->availability() == TypeAvailability::StaticAndAvailabale) {
        auto callee = ASTProxyExpr(position(), typeExpr_->expressionType(), [this](FnCodeGenerator *fncg) {
            auto type = llvm::dyn_cast<llvm::PointerType>(fncg->generator()->llvmTypeForType(typeExpr_->expressionType()));

            auto one = llvm::ConstantInt::get(llvm::Type::getInt32Ty(fncg->generator()->context()), 1);
            auto sizeg = fncg->builder().CreateGEP(llvm::ConstantPointerNull::getNullValue(type), one);
            auto size = fncg->builder().CreatePtrToInt(sizeg, llvm::Type::getInt64Ty(fncg->generator()->context()));

            std::vector<llvm::Value *> args{ size };
            auto alloc = fncg->builder().CreateCall(fncg->generator()->runTimeNew(), args, "alloc");
            return fncg->builder().CreateBitCast(alloc, type);
        });

        auto callGen = InitializationCallCodeGenerator(fncg, CallType::DynamicDispatch);
        return callGen.generate(callee, typeExpr_->expressionType(), args_, name_);
    }
    // TODO: class table lookup
    return nullptr;
}

}  // namespace EmojicodeCompiler
