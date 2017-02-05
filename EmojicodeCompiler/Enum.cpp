//
//  Enum.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Enum.hpp"
#include "CompilerError.hpp"

std::pair<bool, long> Enum::getValueFor(EmojicodeString c) const {
    auto it = map_.find(c);
    if (it == map_.end()) {
        return std::pair<bool, long>(false, 0);
    }
    else {
        return std::pair<bool, long>(true, it->second.first);
    }
}

void Enum::addValueFor(EmojicodeString c, SourcePosition position, EmojicodeString documentation) {
    if (map_.count(c)) {
        throw CompilerError(position, "Duplicate enum value.");
    }
    map_.emplace(c, std::pair<long, EmojicodeString>(valuesCounter++, documentation));
}
