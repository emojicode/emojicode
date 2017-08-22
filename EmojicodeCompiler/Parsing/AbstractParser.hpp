//
//  AbstractParser.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef AbstractParser_hpp
#define AbstractParser_hpp

#include "../Lex/Token.hpp"
#include "../Lex/TokenStream.hpp"
#include "Package.hpp"
#include "../Function.hpp"

namespace EmojicodeCompiler {

struct TypeIdentifier {
    TypeIdentifier(EmojicodeString name, EmojicodeString ns, const Token& token)
        : name(name), ns(ns), token(token) {}
    EmojicodeString name;
    EmojicodeString ns;
    const Token& token;
};

class AbstractParser {
protected:
    AbstractParser(Package *pkg, TokenStream &stream) : package_(pkg), stream_(stream) {};
    Package *package_;
    TokenStream &stream_;

    /// Reads a $type-identifier$
    TypeIdentifier parseTypeIdentifier();
    /// Reads a $type$ and fetches it
    Type parseType(const TypeContext &typeContext, TypeDynamism dynamism);

    /// Parses the arguments for a callable.
    void parseArgumentList(Function *function, const TypeContext &typeContext, bool initializer = false);
    /// Parses the return type for a function if there is one specified.
    void parseReturnType(Function *function, const TypeContext &typeContext);
    void parseGenericArgumentsInDefinition(Function *p, const TypeContext &typeContext);
    void parseGenericArgumentsForType(Type *type, const TypeContext &typeContext, TypeDynamism dynamism, const SourcePosition &p);

    /// Parses and validates the error type
    Type parseErrorEnumType(const TypeContext &typeContext, TypeDynamism dynamism, const SourcePosition &p);
};

}  // namespace EmojicodeCompiler

#endif /* AbstractParser_hpp */
