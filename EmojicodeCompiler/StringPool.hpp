//
//  StringPool.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef StringPool_hpp
#define StringPool_hpp

#include <vector>
#include "EmojicodeCompiler.hpp"

class StringPool {
public:
    /** Returns the globaly used string pool. */
    static StringPool& theStringPool() {
        static StringPool stringPool;
        return stringPool;
    }
    
    StringPool(StringPool const&) = delete;
    void operator=(StringPool const&) = delete;
    
    /**
     * Pools the given string. The pool is searched for an identical strings first, if no such string is found,
     * the string is added to the end of the pool. 
     * @returns The index to access the string in the pool.
     */
    EmojicodeCoin poolString(const EmojicodeString &string);
    /** Returns a vector of all strings in the order in which they must appear in the pool at run-time. */
    const std::vector<EmojicodeString>& strings() const { return strings_; }
private:
    StringPool() {};
    std::vector<EmojicodeString> strings_;
};

#endif /* StringPool_hpp */
