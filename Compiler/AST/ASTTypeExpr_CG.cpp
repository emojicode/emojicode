//
//  ASTTypeExpr_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTTypeExpr.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "Types/Class.hpp"
#include "Types/ValueType.hpp"

namespace EmojicodeCompiler {

Value* ASTTypeFromExpr::generate(FunctionCodeGenerator *fg) const {
    return expr_->generate(fg);
}

Value* ASTStaticType::generate(FunctionCodeGenerator *fg) const {
    if (type_.type() == TypeType::Class) {
        return type_.klass()->classMeta();
    }
    return fg->generator()->valueTypeMetaFor(type_);
}

}  // namespace EmojicodeCompiler
