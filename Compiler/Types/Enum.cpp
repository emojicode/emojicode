//
//  Enum.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Enum.hpp"
#include "CompilerError.hpp"

namespace EmojicodeCompiler {

std::pair<bool, long> Enum::getValueFor(const std::u32string &c) const {
    auto it = map_.find(c);
    if (it == map_.end()) {
        return std::pair<bool, long>(false, 0);
    }
    return std::pair<bool, long>(true, it->second.value);
}

void Enum::addValueFor(const std::u32string &c, const SourcePosition &position, std::u32string documentation) {
    if (map_.count(c) > 0) {
        throw CompilerError(position, "Duplicate enum value.");
    }
    map_.emplace(c, EnumValue{ nextValue_++, documentation });
}

}  // namespace EmojicodeCompiler
