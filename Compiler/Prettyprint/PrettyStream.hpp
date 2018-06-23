//
//  Prettyprinter.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 25/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Prettyprinter_hpp
#define Prettyprinter_hpp

#include "Lex/SourcePosition.hpp"
#include "Types/TypeContext.hpp"
#include <fstream>
#include <functional>
#include <memory>

namespace EmojicodeCompiler {

class ASTType;
class PrettyPrinter;

/// Prettyprinter class produces Emojicode source code from an RecordingPackage.
///
/// Prettyprinter can be appended to  with <<. Prettyprinter then appends data as necessary to the underlying stream.
///
/// Prettyprinter features a concept of "whitespace offers". By a call to offerSpace() or offerNewLine() at whitespace
/// is offered. The whitespace is then appended before the next object passed to << if refuseOffer() isn’t called
/// before.
class PrettyStream {
public:
    PrettyStream(PrettyPrinter *prettyPrinter) : prettyPrinter_(prettyPrinter) {}

    void setOutPath(const std::string &path);

    template <typename T>
    PrettyStream& operator<<(const std::unique_ptr<T> &node) {
        node->toCode(*this);
        return *this;
    }

    template <typename T>
    PrettyStream& operator<<(const std::shared_ptr<T> &node) {
        node->toCode(*this);
        return *this;
    }

    PrettyStream& operator<<(ASTType *node);
    PrettyStream& operator<<(const std::u32string &str);
    PrettyStream& operator<<(const Type &type);

    /// Appends the whitespace offer to the stream if any is available. Then appends rhs and returns this instance.
    template<typename T>
    PrettyStream& operator<<(const T &rhs) {
        if (whitespaceOffer_ != 0) {
            stream_ << whitespaceOffer_;
            whitespaceOffer_ = 0;
        }
        stream_ << rhs;
        return *this;
    }

    PrettyStream& thisStream() { return *this; }

    void printClosure(Function *function);

    void setLastCommentQueryPlace(const SourcePosition &p);
    void printComments(const SourcePosition &p);

    void withTypeContext(const TypeContext &context, std::function<void ()> fn);

    /// Appends the requested amount of indentation characters to the stream and returns it.
    PrettyStream& indent() { return *this << std::string(indentation_ * 2, ' '); }

    void increaseIndent() { indentation_++; }
    void decreaseIndent() { indentation_--; }

    /// Refuses any available whitespace offer.
    /// @returns The instance.
    PrettyStream& refuseOffer() { whitespaceOffer_ = 0; return *this; }
    /// Offers a space character
    void offerSpace() { whitespaceOffer_ = ' '; }
    /// Offers a new line character
    void offerNewLine() { whitespaceOffer_ = '\n'; }
    /// Calls offerSpace() unless collection returns true for empty()
    template<typename T>
    void offerNewLineUnlessEmpty(const T &collection) { if (!collection.empty()) { offerNewLine(); } }
private:
    std::fstream stream_;

    PrettyPrinter *prettyPrinter_;
    TypeContext typeContext_;
    char whitespaceOffer_ = 0;
    unsigned int indentation_ = 0;
    SourcePosition lastCommentQuery_ = SourcePosition(0, 0, nullptr);
};

}  // namespace EmojicodeCompiler


#endif /* Prettyprinter_hpp */
