//
//  SourcePosition.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 01/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef SourcePosition_hpp
#define SourcePosition_hpp

#include <string>
#include <utility>

namespace EmojicodeCompiler {

/// SourcePosition denotes a location in a source code file.
struct SourcePosition {
    SourcePosition(size_t line, size_t character, std::string file) : line(line), character(character),
                                                                      file(std::move(file)) {}
    size_t line;
    size_t character;
    std::string file;
};

}  // namespace EmojicodeCompiler

#endif /* SourcePosition_hpp */
