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
    if (type_->type().type() == TypeType::Class) {
        return type_->type().klass()->reificationFor(type_->type().genericArguments()).classInfo;
    }
    if (type_->type().unboxedType() == TypeType::Protocol) {
        return fg->generator()->protocolIdentifierFor(type_->type());
    }
    return fg->boxInfoFor(type_->type());
}

}  // namespace EmojicodeCompiler
