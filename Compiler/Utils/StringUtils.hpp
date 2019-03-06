//
//  EmojicodeCompiler.h
//  Emojicode
//
//  Created by Theo Weidmann on 01.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef EmojicodeCompiler_hpp
#define EmojicodeCompiler_hpp

#include <codecvt>
#include <locale>
#include <sstream>
#include <string>

namespace EmojicodeCompiler {

inline std::string utf8(const std::u32string &s) {
    return std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t>().to_bytes(s);
}

inline bool endsWith(const std::string &value, const std::string &ending) {
    if (ending.size() > value.size()) {
        return false;
    }
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

template<typename Head>
void appendToStream(std::stringstream &stream, Head head) {
    stream << head;
}

template<typename Head, typename... Args>
void appendToStream(std::stringstream &stream, Head head, Args... args) {
    stream << head;
    appendToStream(stream, args...);
}

}  // namespace EmojicodeCompiler

#endif /* EmojicodeCompiler_hpp */
