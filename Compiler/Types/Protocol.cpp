//
//  Protocol.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Protocol.hpp"
#include <utility>

namespace EmojicodeCompiler {

Protocol::Protocol(std::u32string name, Package *pkg, const SourcePosition &p, const std::u32string &string,
                   bool exported)
    : Generic(std::move(name), pkg, p, string, exported) {}

}  // namespace EmojicodeCompiler
