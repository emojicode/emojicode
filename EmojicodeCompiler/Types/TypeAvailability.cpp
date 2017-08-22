//
//  TypeAvailability.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 13/04/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "TypeAvailability.hpp"
#include "../CompilerError.hpp"

namespace EmojicodeCompiler {

void notStaticError(TypeAvailability t, const SourcePosition &p, const char *name) {
    if (!isStatic(t)) {
        throw CompilerError(p, "%s cannot be used dynamically.", name);
    }
}

}  // namespace EmojicodeCompiler
