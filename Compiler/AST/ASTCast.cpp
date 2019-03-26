//
//  ASTCast.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 25.09.18.
//

#include "ASTCast.hpp"
#include "Analysis/ExpressionAnalyser.hpp"
#include "CompilerError.hpp"
#include "Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

Type ASTCast::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    auto type = analyser->analyseTypeExpr(typeExpr_, expectation);

    Type originalType = analyser->expect(TypeExpectation(), &expr_);

    if (originalType.type() == TypeType::Optional) {
        analyser->error(CompilerError(position(), "Cannot cast optional."));
        return type.optionalized();
    }

    if (originalType.compatibleTo(type, analyser->typeContext())) {
        analyser->error(CompilerError(position(), "Unnecessary cast."));
    }
    else if (!type.compatibleTo(originalType, analyser->typeContext())
             && !(originalType.unboxedType() == TypeType::Protocol && type.unboxedType() == TypeType::Protocol)) {
        auto typeString = type.toString(analyser->typeContext());
        analyser->error(CompilerError(position(), "Cast to unrelated type ", typeString, " will always fail."));
    }

    if (type.type() == TypeType::Class && (originalType.type() == TypeType::Someobject ||
                                           originalType.type() == TypeType::Class)) {
        isDowncast_ = true;
        analyser->comply(originalType, TypeExpectation(false, false), &expr_);
        return type.optionalized();
    }

    analyser->comply(originalType, TypeExpectation(true, false), &expr_);

    if (type.unboxedType() == TypeType::Protocol) {
        if (!type.genericArguments().empty()) {
            analyser->error(CompilerError(position(), "Cannot cast to generic protocols."));
        }
        assert(type.storageType() == StorageType::Box);
        return type.unboxed().boxedFor(type).optionalized();
    }

    if (type.type() != TypeType::Class && type.type() != TypeType::ValueType && type.type() != TypeType::Enum){
        auto typeString = type.toString(analyser->typeContext());
        throw CompilerError(position(), "You cannot cast to ", typeString, ".");
    }
    return type.optionalized().boxedFor(originalType.boxedFor());
}

}  // namespace EmojicodeCompiler
