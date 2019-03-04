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
    /// Constructs a source position that indicates that the position is unknown.
    explicit SourcePosition() : line(0), character(0), file(nullptr) {}
    /// Constructs a source position from the provided information.
    SourcePosition(unsigned int line, unsigned int character, SourceFile *file)
        : line(line), character(character), file(file) {}

    /// Returns true iff the position is not known.
    bool isUnknown() const { return file == nullptr; }

    unsigned int line;
    unsigned int character;
    /// The file into which the this position points. nullptr if the position is unknown.
    SourceFile *file;

    /// @returns The line into which this SourcePosition points or an empty string if the line cannot be
    /// returned for whatever reason.
    std::u32string wholeLine() const;

    /// @returns A string describing the location for use at runtime.
    std::string toRuntimeString() const;
};

}  // namespace EmojicodeCompiler

#endif /* SourcePosition_hpp */
