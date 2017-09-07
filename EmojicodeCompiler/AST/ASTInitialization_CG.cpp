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
#include "../Types/Class.hpp"
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
            InitializationCallCodeGenerator(fncg, CallType::StaticDispatch)
            .generate(vtDestination_->generate(fncg), typeExpr_->expressionType(), args_, name_);
    }
    return nullptr;
}

Value* ASTInitialization::generateClassInit(FnCodeGenerator *fncg) const {
    if (typeExpr_->availability() == TypeAvailability::StaticAndAvailabale) {
        auto type = llvm::dyn_cast<llvm::PointerType>(fncg->typeHelper().llvmTypeFor(typeExpr_->expressionType()));
        auto size = fncg->sizeFor(type);

        auto alloc = fncg->builder().CreateCall(fncg->generator()->runTimeNew(), size, "alloc");
        auto obj = fncg->builder().CreateBitCast(alloc, type);

        fncg->builder().CreateStore(typeExpr_->expressionType().eclass()->classMeta(), fncg->getObjectMetaPtr(obj));

        auto callGen = InitializationCallCodeGenerator(fncg, CallType::StaticDispatch);
        return callGen.generate(obj, typeExpr_->expressionType(), args_, name_);
    }
    // TODO: class table lookup
    return nullptr;
}

}  // namespace EmojicodeCompiler
