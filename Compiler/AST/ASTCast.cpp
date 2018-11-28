//
//  ASTCast.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 25.09.18.
//

#include "ASTCast.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "CompilerError.hpp"
#include "Types/TypeExpectation.hpp"
#include "Compiler.hpp"

namespace EmojicodeCompiler {

Type ASTCast::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    auto type = analyser->analyseTypeExpr(typeExpr_, expectation);

    Type originalType = analyser->expect(TypeExpectation(), &expr_);
    if (originalType.compatibleTo(type, analyser->typeContext())) {
        analyser->compiler()->error(CompilerError(position(), "Unnecessary cast."));
    }
    else if (!type.compatibleTo(originalType, analyser->typeContext())
             && !(originalType.unboxedType() == TypeType::Protocol && type.unboxedType() == TypeType::Protocol)) {
        auto typeString = type.toString(analyser->typeContext());
        analyser->compiler()->error(CompilerError(position(), "Cast to unrelated type ", typeString,
                                                  " will always fail."));
    }

    if (type.type() == TypeType::Class) {
        if (!type.genericArguments().empty()) {
            analyser->compiler()->error(CompilerError(position(),
                                                      "Class casts with generic arguments are not available."));
        }

        if (originalType.type() == TypeType::Someobject || originalType.type() == TypeType::Class) {
            castType_ = CastType::ClassDowncast;
            return type.optionalized();
        }

        castType_ = CastType::ToClass;
    }
    else if (type.unboxedType() == TypeType::Protocol) {
        if (!type.genericArguments().empty()) {
            analyser->compiler()->error(CompilerError(position(), "Cannot cast to generic protocols."));
        }
        castType_ = CastType::ToProtocol;
        assert(type.storageType() == StorageType::Box);
        return type.unboxed().boxedFor(type).optionalized();
    }
    else if (type.type() == TypeType::ValueType || type.type() == TypeType::Enum) {
        castType_ = CastType::ToValueType;
    }
    else {
        auto typeString = type.toString(analyser->typeContext());
        throw CompilerError(position(), "You cannot cast to ", typeString, ".");
    }

    return type.optionalized().boxedFor(originalType.boxedFor());
}

}  // namespace EmojicodeCompiler
