//
//  Enum.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Enum.hpp"
#include "../CompilerError.hpp"

namespace EmojicodeCompiler {

std::pair<bool, long> Enum::getValueFor(const EmojicodeString &c) const {
    auto it = map_.find(c);
    if (it == map_.end()) {
        return std::pair<bool, long>(false, 0);
    }
    return std::pair<bool, long>(true, it->second.first);
}

void Enum::addValueFor(const EmojicodeString &c, const SourcePosition &position, EmojicodeString documentation) {
    if (map_.count(c) > 0) {
        throw CompilerError(position, "Duplicate enum value.");
    }
    map_.emplace(c, std::pair<long, EmojicodeString>(valuesCounter++, documentation));
}

}  // namespace EmojicodeCompiler
