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

class SourceFile;

/// SourcePosition denotes a location in a source code file.
struct SourcePosition {
    SourcePosition(size_t line, size_t character, SourceFile *file) : line(line), character(character), file(file) {}

    size_t line;
    size_t character;
    SourceFile *file;

    /// @returns The line into which this SourcePosition points or an empty string if the line cannot be
    /// returned for whatever reason.
    std::u32string wholeLine() const;
};

}  // namespace EmojicodeCompiler

#endif /* SourcePosition_hpp */
