//
//  STIProvider.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 14/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "STIProvider.hpp"

namespace EmojicodeCompiler {

int STIProvider::nextVti_ = 0;
STIProvider STIProvider::globalStiProvider;

int STIProvider::next() {
    return STIProvider::nextVti_++;
}

}  // namespace EmojicodeCompiler
