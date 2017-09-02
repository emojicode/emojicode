//
//  ProtocolTypeBodyParser.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 02/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ProtocolTypeBodyParser.hpp"
#include "../Functions/ProtocolFunction.hpp"
#include "../Types/Protocol.hpp"

namespace EmojicodeCompiler {

void ProtocolTypeBodyParser::parseMethod(const std::u32string &name, TypeBodyAttributeParser attributes,
                                         const Documentation &documentation, AccessLevel access,
                                         const SourcePosition &p) {
    auto method = new ProtocolFunction(name, AccessLevel::Public, false, owningType(), package_,
                                       p, false, documentation.get(), attributes.has(Attribute::Deprecated), false,
                                       FunctionType::ObjectMethod);
    parseParameters(method, TypeContext(type_), false);
    parseReturnType(method, TypeContext(type_));

    type_.protocol()->addMethod(method);
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
