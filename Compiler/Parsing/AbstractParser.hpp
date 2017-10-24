//
//  AbstractParser.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef AbstractParser_hpp
#define AbstractParser_hpp

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
    Documentation& parse(TokenStream *tokenStream) {
        if (tokenStream->nextTokenIs(TokenType::DocumentationComment)) {
            auto &token = tokenStream->consumeToken(TokenType::DocumentationComment);
            position_ = token.position();
            documentation_ = token.value();
            found_ = true;
        }
        return *this;
    }
    const std::u32string& get() const { return documentation_; }
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

enum class TypeDynamism {
    /** No dynamism is allowed or no dynamism was used. */
    None = 0,
    /** No kind of dynamism is allowed. This value never comes from a call to @c parseAndFetchType . */
    AllKinds = 0b11,
    /** Generic Variables are allowed or were used. */
    GenericTypeVariables = 0b1,
    /** Self is allowed or was used. */
    Self = 0b10
};

inline TypeDynamism operator&(TypeDynamism a, TypeDynamism b) {
    return static_cast<TypeDynamism>(static_cast<int>(a) & static_cast<int>(b));
}

inline TypeDynamism operator|(TypeDynamism a, TypeDynamism b) {
    return static_cast<TypeDynamism>(static_cast<int>(a) | static_cast<int>(b));
}

class AbstractParser {
protected:
    AbstractParser(Package *pkg, TokenStream &stream) : package_(pkg), stream_(stream) {};
    Package *package_;
    TokenStream &stream_;

    /// Reads a $type-identifier$
    TypeIdentifier parseTypeIdentifier();
    /// Reads a $type$ and fetches it
    Type parseType(const TypeContext &typeContext, TypeDynamism dynamism);
    /// Parses $generic-parameters$
    template <typename T>
    void parseGenericParameters(Generic<T> *generic, const TypeContext &typeContext) {
        while (stream_.consumeTokenIf(E_SPIRAL_SHELL)) {
            auto &variable = stream_.consumeToken(TokenType::Variable);
            auto constraint = parseType(typeContext, TypeDynamism::GenericTypeVariables);
            generic->addGenericArgument(variable.value(), constraint, variable.position());
        }
    }

    /// Parses $parameters$ for a function if there are any specified.
    /// @param initializer If this is true, the method parses $init-parameters$ instead.
    void parseParameters(Function *function, const TypeContext &typeContext, bool initializer = false);
    /// Parses a $return-type$ for a function one is specified.
    void parseReturnType(Function *function, const TypeContext &typeContext);
    /// Parses $generic-arguments$ for a type.
    void parseGenericArgumentsForType(Type *type, const TypeContext &typeContext, TypeDynamism dynamism,
                                      const SourcePosition &p);

    /// Parses and validates the error type
    Type parseErrorEnumType(const TypeContext &typeContext, TypeDynamism dynamism, const SourcePosition &p);

    std::unique_ptr<FunctionParser> factorFunctionParser(Package *pkg, TokenStream &stream, TypeContext context,
                                                         Function *function);
private:
    /// Parses a $multi-protocol$
    Type parseMultiProtocol(bool optional, const TypeContext &typeContext, TypeDynamism dynamism);
    /// Parses a $callable-type$. The first token has already been consumed.
    Type parseCallableType(bool optional, const TypeContext &typeContext, TypeDynamism dynamism);

    Type parseGenericVariable(bool optional, const TypeContext &typeContext, TypeDynamism dynamism);

    Type parseErrorType(bool optional, const TypeContext &typeContext, TypeDynamism dynamism);
};

}  // namespace EmojicodeCompiler

#endif /* AbstractParser_hpp */
