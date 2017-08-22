//
//  ProtocolFunction.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 14/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ProtocolFunction.hpp"

namespace EmojicodeCompiler {

void ProtocolFunction::assignVti() {
    if (!assigned()) {
        for (Function *function : overriders_) {
            function->assignVti();
        }
        assigned_ = true;
    }
}

void ProtocolFunction::setUsed(bool) {
    if (!used_) {
        used_ = true;
        for (Function *function : overriders_) {
            function->setUsed();
        }
    }
}

}  // namespace EmojicodeCompiler
