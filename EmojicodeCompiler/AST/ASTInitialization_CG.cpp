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

namespace EmojicodeCompiler {

Value* ASTInitialization::generateExpr(FnCodeGenerator *fncg) const {
    switch (initType_) {
        case InitType::Class:
            InitializationCallCodeGenerator(fncg, CallType::DynamicDispatch).generate(*typeExpr_,
                                                                                      typeExpr_->expressionType(),
                                                                                      args_, name_);
            break;
        case InitType::Enum:
            return llvm::ConstantInt::get(llvm::Type::getInt64Ty(fncg->generator()->context()),
                                          typeExpr_->expressionType().eenum()->getValueFor(name_).second);
            break;
        case InitType::ValueType:
            VTInitializationCallCodeGenerator(fncg).generate(vtDestination_, typeExpr_->expressionType(), args_, name_);
    }
    return nullptr;
}

}  // namespace EmojicodeCompiler
