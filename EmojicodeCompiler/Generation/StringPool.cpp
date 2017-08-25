//
//  StringPool.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "StringPool.hpp"

namespace EmojicodeCompiler {

EmojicodeInstruction StringPool::pool(const std::u32string &string) {
    auto it = pool_.find(string);
    if (it != pool_.end()) {
        return it->second;
    }

    strings_.emplace_back(string);
    auto index = static_cast<EmojicodeInstruction>(strings_.size() - 1);
    pool_.emplace(string, index);
    return index;
}

}  // namespace EmojicodeCompiler
