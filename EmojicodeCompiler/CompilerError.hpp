//
//  CompilerError.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 30/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CompilerError_hpp
#define CompilerError_hpp

#include "EmojicodeCompiler.hpp"
#include "Lex/SourcePosition.hpp"
#include <exception>
#include <sstream>

namespace EmojicodeCompiler {

/// A CompilerError represents an error in an Emojicode source document.
/// Although this class inherits from std::exception not all errors are thrown. If an error does not severly affect
/// the rest of the nearby code and the compiler can safely continue the compilation the error is directly passed
/// to Application::error. If an error is thrown, the catching catch must pass the error to Application::error.
class CompilerError: public std::exception {
public:
    template<typename... Args>
    explicit CompilerError(SourcePosition p, Args... args) : position_(std::move(p)) {
        std::stringstream stream;
        appendToStream(stream, args...);
        message_ = stream.str();
    }

    const SourcePosition& position() const { return position_; }
    const std::string& message() const { return message_; }
private:
    SourcePosition position_;
    std::string message_;
};

}  // namespace EmojicodeCompiler

#endif /* CompilerError_hpp */
