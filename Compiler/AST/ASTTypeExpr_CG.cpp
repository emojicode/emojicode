//
//  ASTTypeExpr_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTTypeExpr.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "Generation/TypeDescriptionGenerator.hpp"
#include "ASTType.hpp"

namespace EmojicodeCompiler {

Value* ASTTypeFromExpr::generate(FunctionCodeGenerator *fg) const {
    return expr_->generate(fg);
}

Value* ASTStaticType::generate(FunctionCodeGenerator *fg) const {
    return TypeDescriptionGenerator(fg, TypeDescriptionUser::Function).generate(type_->type());
}

}  // namespace EmojicodeCompiler
