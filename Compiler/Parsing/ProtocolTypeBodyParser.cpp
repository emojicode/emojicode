//
//  ProtocolTypeBodyParser.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 02/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ProtocolTypeBodyParser.hpp"
#include "Types/Protocol.hpp"

namespace EmojicodeCompiler {

void ProtocolTypeBodyParser::parseMethod(const std::u32string &name, TypeBodyAttributeParser attributes,
                                         const Documentation &documentation, AccessLevel access, bool imperative,
                                         const SourcePosition &p) {
    auto method = std::make_unique<Function>(name, AccessLevel::Public, false, owningType(), package_,
                                             p, false, documentation.get(),
                                             attributes.has(Attribute::Deprecated), false, imperative,
                                             FunctionType::ObjectMethod);
    parseParameters(method.get(), TypeContext(owningType()), false);
    parseReturnType(method.get(), TypeContext(owningType()));

    owningType().protocol()->addMethod(std::move(method));
}

Initializer* ProtocolTypeBodyParser::parseInitializer(const std::u32string &name, TypeBodyAttributeParser attributes,
                                                      const Documentation &documentation, AccessLevel access,
                                                      const SourcePosition &p) {
    throw CompilerError(p, "Only method declarations are allowed inside a protocol.");
}

void ProtocolTypeBodyParser::parseInstanceVariable(const SourcePosition &p) {
    throw CompilerError(p, "Only method declarations are allowed inside a protocol.");
}

void ProtocolTypeBodyParser::parseProtocolConformance(const SourcePosition &p) {
    throw CompilerError(p, "Only method declarations are allowed inside a protocol.");
}

}  // namespace EmojicodeCompiler
