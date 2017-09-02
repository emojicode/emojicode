//
//  ProtocolTypeBodyParser.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 02/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ProtocolTypeBodyParser_hpp
#define ProtocolTypeBodyParser_hpp

#include "TypeBodyParser.hpp"

namespace EmojicodeCompiler {

class ProtocolTypeBodyParser : public TypeBodyParser {
    using TypeBodyParser::TypeBodyParser;
private:
    void parseMethod(const std::u32string &name, TypeBodyAttributeParser attributes, const Documentation &documentation,
                     AccessLevel access, const SourcePosition &p) override;
    Initializer* parseInitializer(TypeBodyAttributeParser attributes, const Documentation &documentation,
                                  AccessLevel access, const SourcePosition &p) override;
    void parseInstanceVariable(const SourcePosition &p) override;
    void parseProtocolConformance(const SourcePosition &p) override;
};

}  // namespace EmojicodeCompiler

#endif /* ProtocolTypeBodyParser_hpp */
