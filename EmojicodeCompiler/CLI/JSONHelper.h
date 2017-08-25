//
//  JSONHelper.h
//  Emojicode
//
//  Created by Theo Weidmann on 25/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef JSONHelper_h
#define JSONHelper_h

#include <ostream>
#include <string>

namespace EmojicodeCompiler {

namespace CLI {

class CommaPrinter {
public:
    void print() {
        if (printedFirst_) {
            putc(',', stdout);
        }
        printedFirst_ = true;
    }
private:
    bool printedFirst_ = false;
};

inline void jsonString(const std::string &string, std::ostream &output) {
    output << '"';
    for (auto c : string) {
        switch (c) {
            case '\\':
                output << "\\\\";
                break;
            case '"':
                output << "\\\"";
                break;
            case '/':
                output << "\\/";
                break;
            case '\b':
                output << "\\b";
                break;
            case '\f':
                output << "\\f";
                break;
            case '\n':
                output << "\\n";
                break;
            case '\r':
                output << "\\r";
                break;
            case '\t':
                output << "\\t";
                break;
            default:
                output << c;
        }
    }
    output << '"';
}

}  // namespace CLI
    
}  // namespace EmojicodeCompiler

#endif /* JSONHelper_h */
