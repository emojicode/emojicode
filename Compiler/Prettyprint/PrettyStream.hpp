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

class ASTNode;
class PrettyPrinter;

/// PrettyStream manages the stream to which code is appended. PrettyStream can be appended to with <<.
///
/// PrettyStream features a concept of "whitespace offers". By a call to offerSpace() or offerNewLine() whitespace
/// is offered. The whitespace is then appended before the next object passed to << unless refuseOffer() is called
/// previously.
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

    PrettyStream& operator<<(const std::string &rhs);
    PrettyStream& operator<<(const std::u32string &str);
    PrettyStream& operator<<(ASTNode *node);
    PrettyStream& operator<<(const ASTNode &node);
    PrettyStream& operator<<(const Type &type);

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
    SourcePosition lastCommentQuery_ = SourcePosition();
};

}  // namespace EmojicodeCompiler


#endif /* Prettyprinter_hpp */
