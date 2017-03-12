//
//  AbstractParser.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef AbstractParser_hpp
#define AbstractParser_hpp

#include "Token.hpp"
#include "TokenStream.hpp"
#include "Package.hpp"
#include "Function.hpp"

struct ParsedType {
    ParsedType(EmojicodeString name, EmojicodeString ns, const Token& token)
        : name(name), ns(ns), token(token) {}
    EmojicodeString name;
    EmojicodeString ns;
    const Token& token;
};

class AbstractParser {
protected:
    AbstractParser(Package *pkg, TokenStream stream) : package_(pkg), stream_(stream) {};
    Package *package_;
    TokenStream stream_;

    /// Reads a type
    ParsedType parseType();
    /** Reads a type name and stores it into the given pointers. */
    Type parseTypeDeclarative(const TypeContext &typeContext, TypeDynamism dynamism, Type expectation = Type::nothingness(),
                              TypeDynamism *dynamicType = nullptr);

    /// Parses the arguments for a callable.
    void parseArgumentList(Function *function, const TypeContext &typeContext, bool initializer = false);
    /// Parses the return type for a function if there is one specified.
    void parseReturnType(Function *function, const TypeContext &typeContext);
    void parseGenericArgumentsInDefinition(Function *p, const TypeContext &typeContext);
    /// Parses the body of a function which might either be a code block or a linking table index
    void parseBody(Function *function, bool allowNative);
    /// Parses the code block body of a function
    void parseBodyBlock(Function *function);
    void parseGenericArgumentsForType(Type *type, const TypeContext &typeContext, TypeDynamism dynamism, SourcePosition p);

    /// Parses and validates the error type
    Type parseErrorEnumType(const TypeContext &typeContext, TypeDynamism dynamism, const SourcePosition &p);
};

#endif /* AbstractParser_hpp */
