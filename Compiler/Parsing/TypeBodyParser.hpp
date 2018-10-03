//
//  TypeBodyParser.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 24/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef TypeBodyParser_hpp
#define TypeBodyParser_hpp

#include "AbstractParser.hpp"
#include "AttributesParser.hpp"

namespace EmojicodeCompiler {

enum class AccessLevel;
class Initializer;
class CompilerError;

using TypeBodyAttributeParser = AttributeParser<Attribute::Deprecated, Attribute::Final, Attribute::Override,
    Attribute::StaticOnType, Attribute::Unsafe, Attribute::Mutating, Attribute::Required, Attribute::Escaping>;

/// TypeBodyParser parses $type-body$s of $type-definition$s, which are
/// represented by TypeDefinition. Some methods of this class are specialized for some types.
template <typename TypeDef>
class TypeBodyParser : protected AbstractParser {
public:
    TypeBodyParser(TypeDef *typeDef, Package *pkg, TokenStream &stream, bool interface)
        : AbstractParser(pkg, stream), typeDef_(typeDef), interface_(interface) {}

    void parse();

protected:
    /// Called if an $protocol-conformance$ has been detected. The first token has already been parsed.
    void parseProtocolConformance(const SourcePosition &p);
    /// Called if an $enum-value$ has been detected. The first token has already been parsed.
    void parseEnumValue(const SourcePosition &p, const Documentation &documentation);
    /// Called if an $instance-variable$ has been detected. The first token has already been parsed.
    void parseInstanceVariable(const SourcePosition &p);
    /// Called if a $method$ has been detected. All tokens up to and including the name.
    void parseMethod(const std::u32string &name, TypeBodyAttributeParser attributes,
                     const Documentation &documentation, AccessLevel access, bool imperative,
                     const SourcePosition &p);
    /// Called if an $initializer$ has been detected. The first token has already been parsed.
    Initializer* parseInitializer(const std::u32string &name, TypeBodyAttributeParser attributes,
                                  const Documentation &documentation, AccessLevel access,
                                  const SourcePosition &p);

    void parseDeinitializer(const SourcePosition &p);

    void doParseMethod(const std::u32string &name, TypeBodyAttributeParser attributes,
                       const Documentation &documentation, AccessLevel access, bool imperative,
                       const SourcePosition &p);

    Initializer* doParseInitializer(const std::u32string &name, TypeBodyAttributeParser attributes,
                                    const Documentation &documentation, AccessLevel access,
                                    const SourcePosition &p);

private:
    TypeDef *typeDef_;
    bool interface_;
    AccessLevel readAccessLevel();
    void parseFunctionBody(Function *function);
    void parseFunction(Function *function, bool inititalizer, bool escaping);
};

}  // namespace EmojicodeCompiler

#endif /* TypeBodyParser_hpp */
