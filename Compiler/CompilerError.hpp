//
//  CompilerError.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 30/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CompilerError_hpp
#define CompilerError_hpp

#include "Lex/SourcePosition.hpp"
#include "Utils/StringUtils.hpp"
#include <exception>
#include <sstream>
#include <vector>

namespace EmojicodeCompiler {

struct Note {
    Note(SourcePosition p, std::string m) : position(p), message(std::move(m)) {}
    SourcePosition position;
    std::string message;
};

/// A CompilerError represents an error in an Emojicode source document.
/// Although this class inherits from std::exception not all errors should be thrown. If an error does not fatally
/// affect compilation and the compiler can safely continue the compilation the error should directly be passed
/// to Compiler::error. If an error is thrown, the catching catch must pass the error to Compiler::error.
///
/// Errors are caught in the following locations:
/// - FunctionParser catches errors on a per-statement-basis, tries to recover and continue after the end of
///   the current block
/// - SemanticAnalyser catches on a per-function-basis, skips to the next function
/// - Compiler, if the Compiler catches an error the compilation is aborted immediately.
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
    const std::vector<Note>& notes() const { return notes_; }

    template<typename... Args>
    void addNotes(SourcePosition p, Args... args) {
        std::stringstream stream;
        appendToStream(stream, args...);
        notes_.emplace_back(p, stream.str());
    }

private:
    SourcePosition position_;
    std::string message_;
    std::vector<Note> notes_;
};

}  // namespace EmojicodeCompiler

#endif /* CompilerError_hpp */
