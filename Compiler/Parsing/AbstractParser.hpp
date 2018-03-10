//
//  AbstractParser.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef AbstractParser_hpp
#define AbstractParser_hpp

#include "Emojis.h"
#include "Lex/TokenStream.hpp"
#include "Types/Generic.hpp"
#include "Types/TypeContext.hpp"
#include <memory>
#include <utility>

namespace EmojicodeCompiler {

class Function;

class Package;

class FunctionParser;

class Documentation {
public:
    Documentation &parse(TokenStream *tokenStream) {
        if (tokenStream->nextTokenIs(TokenType::DocumentationComment)) {
            auto token = tokenStream->consumeToken(TokenType::DocumentationComment);
            position_ = token.position();
            documentation_ = token.value();
            found_ = true;
        }
        return *this;
    }

    const std::u32string &get() const { return documentation_; }

    void disallow() const {
        if (found_) {
            throw CompilerError(position_, "Misplaced documentation token.");
        }
    }

private:
    std::u32string documentation_;
    bool found_ = false;
    SourcePosition position_ = SourcePosition(0, 0, "");
};

struct TypeIdentifier {
    TypeIdentifier(std::u32string name, std::u32string ns, SourcePosition p)
            : name(std::move(name)), ns(std::move(ns)), position(std::move(p)) {}

    std::u32string name;
    std::u32string ns;
    SourcePosition position;
};

class AbstractParser {
protected:
    AbstractParser(Package *pkg, TokenStream &stream) : package_(pkg), stream_(stream) {}

    Package *package_;
    TokenStream &stream_;

    /// Reads a $type-identifier$
    TypeIdentifier parseTypeIdentifier();

    /// Reads a $type$ and fetches it
    Type parseType(const TypeContext &typeContext);

    /// Parses $generic-parameters$
    template<typename T, typename E>
    void parseGenericParameters(Generic<T, E> *generic, const TypeContext &typeContext) {
        while (stream_.consumeTokenIf(E_SPIRAL_SHELL)) {
            bool rejectBoxing = stream_.consumeTokenIf(TokenType::Unsafe);
            auto variable = stream_.consumeToken(TokenType::Variable);
            auto constraint = parseType(typeContext);
            generic->addGenericParameter(variable.value(), constraint, rejectBoxing, variable.position());
        }
    }

    /// Parses $parameters$ for a function if there are any specified.
    /// @param initializer If this is true, the method parses $init-parameters$ instead.
    void parseParameters(Function *function, const TypeContext &typeContext, bool initializer = false);

    /// Parses a $return-type$ for a function one is specified.
    void parseReturnType(Function *function, const TypeContext &typeContext);

    /// Parses $generic-arguments$ for a type.
    void parseGenericArgumentsForType(Type *type, const TypeContext &typeContext, const SourcePosition &p);

    /// Parses and validates the error type
    Type parseErrorEnumType(const TypeContext &typeContext, const SourcePosition &p);

    std::unique_ptr<FunctionParser> factorFunctionParser(Package *pkg, TokenStream &stream, TypeContext context,
                                                         Function *function);

private:
    /// Parses a $multi-protocol$
    Type parseMultiProtocol(bool optional, const TypeContext &typeContext);

    /// Parses a $callable-type$. The first token has already been consumed.
    Type parseCallableType(bool optional, const TypeContext &typeContext);

    Type parseGenericVariable(bool optional, const TypeContext &typeContext);

    Type parseErrorType(bool optional, const TypeContext &typeContext);

    /// Parses a $type-main$
    Type parseTypeMain(bool optional, const TypeContext &typeContext);

    Type parseMetatype(const TypeContext &typeContext);
};

}  // namespace EmojicodeCompiler

#endif /* AbstractParser_hpp */
