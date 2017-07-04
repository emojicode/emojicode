//
//  Scoper.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 14/11/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Scoper.hpp"

namespace EmojicodeCompiler {

void Scoper::syncSize() {
    if (nextOffset_ > size_) {
        size_ = nextOffset_;
    }
}

int Scoper::reserveVariable(int size) {
    auto id = nextOffset_;
    nextOffset_ += size;
    syncSize();
    return id;
}

void Scoper::reduceOffsetBy(int size) {
    nextOffset_ -= size;
}

};  // namespace EmojicodeCompiler
