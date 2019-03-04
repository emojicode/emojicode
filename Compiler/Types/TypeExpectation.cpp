//
//  TypeExpectation.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 12/03/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "TypeExpectation.hpp"

namespace EmojicodeCompiler {

bool TypeExpectation::requiresBox(const Type &type) const {
    switch (type.type()) {
        case TypeType::Optional:
            return requiresBox(type.optionalType().unboxed());
        case TypeType::Something:
        case TypeType::Protocol:
        case TypeType::MultiProtocol:
            return true;
        case TypeType::GenericVariable:
        case TypeType::LocalGenericVariable:
            return true;  // TODO: this doesn't seem right
        default:
            return false;
    }
}

}  // namespace EmojicodeCompiler
