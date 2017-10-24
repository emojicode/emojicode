//
//  ASTInitialization.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTInitialization.hpp"
#include "Analysis/SemanticAnalyser.hpp"
#include "Functions/Initializer.hpp"
#include "Types/Enum.hpp"

namespace EmojicodeCompiler {

Type ASTInitialization::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    auto type = analyser->analyseTypeExpr(typeExpr_, expectation);

    if (type.type() == TypeType::Enum) {
        initType_ = InitType::Enum;
        notStaticError(typeExpr_->availability(), position(), "Enums");

        auto v = type.eenum()->getValueFor(name_);
        if (!v.first) {
            throw CompilerError(position(), type.toString(analyser->typeContext()), " does not have a member named ",
                                utf8(name_), ".");
        }
        return type;
    }

    if (type.type() == TypeType::ValueType) {
        initType_ = InitType::ValueType;
        type.setMutable(expectation.isMutable());
        notStaticError(typeExpr_->availability(), position(), "Value Types");
    }

    auto initializer = type.typeDefinition()->getInitializer(name_, type, analyser->typeContext(), position());
    analyser->analyseFunctionCall(&args_, type, initializer);
    return initializer->constructedType(type);
}

}  // namespace EmojicodeCompiler
