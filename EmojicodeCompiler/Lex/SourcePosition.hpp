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

namespace EmojicodeCompiler {

struct SourcePosition {
    SourcePosition(size_t line, size_t character, std::string file) : line(line), character(character), file(file) {};
    size_t line;
    size_t character;
    std::string file;
};

}  // namespace EmojicodeCompiler

#endif /* SourcePosition_hpp */
