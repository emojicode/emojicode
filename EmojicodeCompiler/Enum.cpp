//
//  Enum.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Enum.hpp"
#include "CompilerErrorException.hpp"

std::pair<bool, EmojicodeInteger> Enum::getValueFor(EmojicodeChar c) const {
    auto it = map_.find(c);
    if (it == map_.end()) {
        return std::pair<bool, EmojicodeInteger>(false, 0);
    }
    else {
        return std::pair<bool, EmojicodeInteger>(true, it->second.first);
    }
}

void Enum::addValueFor(EmojicodeChar c, SourcePosition position, EmojicodeString documentation) {
    if (map_.count(c)) {
        throw CompilerErrorException(position, "Duplicate enum value.");
    }
    map_.emplace(c, std::pair<EmojicodeInteger, EmojicodeString>(valuesCounter++, documentation));
}