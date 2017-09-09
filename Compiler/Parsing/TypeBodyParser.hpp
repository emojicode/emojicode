//
//  TypeBodyParser.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 24/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef TypeBodyParser_hpp
#define TypeBodyParser_hpp

#include "../Functions/Function.hpp"
#include "AbstractParser.hpp"
#include "AttributesParser.hpp"
#include <set>

namespace EmojicodeCompiler {

class Initializer;

using TypeBodyAttributeParser = AttributeParser<Attribute::Deprecated, Attribute::Final, Attribute::Override,
    Attribute::StaticOnType, Attribute::Mutating, Attribute::Required>;

/// Instances of subclasses of TypeBodyParser are used to parse the $type-body$ of $type-definition$s, which are
/// represented by TypeDefinition. This class is abstract.
class TypeBodyParser : protected AbstractParser {
public:
    TypeBodyParser(Type type, Package *pkg, TokenStream &stream) : AbstractParser(pkg, stream), type_(std::move(type)) {

    }

    virtual void parse();
    virtual ~TypeBodyParser() = 0;
protected:
    /// Called if an $protocol-conformance$ has been detected. The first token has already been parsed.
    virtual void parseProtocolConformance(const SourcePosition &p);
    /// Called if an $enum-value$ has been detected. The first token has already been parsed.
    virtual void parseEnumValue(const SourcePosition &p, const Documentation &documentation);
    /// Called if an $instance-variable$ has been detected. The first token has already been parsed.
    virtual void parseInstanceVariable(const SourcePosition &p);
    /// Called if a $method$ has been detected. All tokens up to and including the name.
    virtual void parseMethod(const std::u32string &name, TypeBodyAttributeParser attributes,
                             const Documentation &documentation, AccessLevel access, const SourcePosition &p);
    /// Called if an $initializer$ has been detected. The first token has already been parsed.
    virtual Initializer* parseInitializer(const std::u32string &name, TypeBodyAttributeParser attributes,
                                          const Documentation &documentation, AccessLevel access,
                                          const SourcePosition &p);

    Type owningType() {
        if (type_.type() == TypeType::Extension) {
            return dynamic_cast<Extension *>(type_.typeDefinition())->extendedType();
        }
        return type_;
    }
    
    Type type_;
private:
    AccessLevel readAccessLevel();
    void parseFunctionBody(Function *function);
    void parseFunction(Function *function, bool inititalizer);
};

class EnumTypeBodyParser : public TypeBodyParser {
private:
    using TypeBodyParser::TypeBodyParser;
    void parseEnumValue(const SourcePosition &p, const Documentation &documentation) override;
    void parseInstanceVariable(const SourcePosition &p) override;
    Initializer* parseInitializer(const std::u32string &name, TypeBodyAttributeParser attributes,
                                  const Documentation &documentation, AccessLevel access,
                                  const SourcePosition &p) override;
};

class ValueTypeBodyParser : public TypeBodyParser {
private:
    using TypeBodyParser::TypeBodyParser;
    void parseMethod(const std::u32string &name, TypeBodyAttributeParser attributes, const Documentation &documentation,
                     AccessLevel access, const SourcePosition &p) override;
};

class ClassTypeBodyParser : public TypeBodyParser {
public:
    ClassTypeBodyParser(Type type, Package *pkg, TokenStream &stream, std::set<std::u32string> requiredInits)
    : TypeBodyParser(std::move(type), pkg, stream), requiredInitializers_(std::move(requiredInits)) {}
    void parse() override;
private:
    void parseMethod(const std::u32string &name, TypeBodyAttributeParser attributes, const Documentation &documentation,
                     AccessLevel access, const SourcePosition &p) override;
    Initializer* parseInitializer(const std::u32string &name, TypeBodyAttributeParser attributes,
                                  const Documentation &documentation, AccessLevel access,
                                  const SourcePosition &p) override;

    std::set<std::u32string> requiredInitializers_;
};

}  // namespace EmojicodeCompiler

#endif /* TypeBodyParser_hpp */
