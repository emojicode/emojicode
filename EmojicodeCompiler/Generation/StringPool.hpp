//
//  StringPool.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef StringPool_hpp
#define StringPool_hpp

#include "../EmojicodeCompiler.hpp"
#include <map>
#include <vector>

namespace EmojicodeCompiler {

/// StringPool represents an application’s string pool.
class StringPool {
public:
    /// Pools the given string. The pool is searched for an identical strings first, if no such string is found,
    /// the string is added to the end of the pool.
    /// @returns The index to access the string in the pool.
    EmojicodeInstruction pool(const std::u32string &string);
    /// Returns a vector of all strings in the order in which they must appear in the pool at run-time.
    const std::vector<std::u32string>& strings() const { return strings_; }
private:
    std::map<std::u32string, EmojicodeInstruction> pool_;
    std::vector<std::u32string> strings_;
};

}  // namespace EmojicodeCompiler

#endif /* StringPool_hpp */
