//
//  SourcePosition.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 01/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "SourcePosition.hpp"
#include "SourceManager.hpp"

namespace EmojicodeCompiler {

std::u32string SourcePosition::wholeLine() const {
    if (file == nullptr) {
        return std::u32string();
    }

    auto begin = file->lines()[line - 1];
    auto length = line < file->lines().size() ? file->lines()[line] - begin
                                              : file->file().find_first_of('\n', begin) + 1 - begin;
    return file->file().substr(begin, length);
}

}
