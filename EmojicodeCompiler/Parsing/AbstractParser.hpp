//
//  AbstractParser.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef AbstractParser_hpp
#define AbstractParser_hpp

#include <utility>
#include "../Types/TypeContext.hpp"
#include "../Lex/TokenStream.hpp"

namespace EmojicodeCompiler {

class Function;
class Package;

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

    /// Parses the arguments for a callable.
    void parseArgumentList(Function *function, const TypeContext &typeContext, bool initializer = false);
    /// Parses the return type for a function if there is one specified.
    void parseReturnType(Function *function, const TypeContext &typeContext);
    void parseGenericArgumentsInDefinition(Function *function, const TypeContext &typeContext);
    void parseGenericArgumentsForType(Type *type, const TypeContext &typeContext, TypeDynamism dynamism,
                                      const SourcePosition &p);

    /// Parses and validates the error type
    Type parseErrorEnumType(const TypeContext &typeContext, TypeDynamism dynamism, const SourcePosition &p);
};

}  // namespace EmojicodeCompiler

#endif /* AbstractParser_hpp */
