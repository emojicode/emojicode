//
//  Protocol.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Protocol_hpp
#define Protocol_hpp

#include "TypeDefinition.hpp"

namespace EmojicodeCompiler {

class Package;

struct ProtocolReification : TypeDefinitionReification {};

class Protocol : public Generic<Protocol, ProtocolReification, TypeDefinition> {
public:
    Protocol(std::u32string name, Package *pkg, const SourcePosition &p, const std::u32string &string, bool exported);

    Type type() override { return Type(this); }

    bool canResolve(TypeDefinition *resolutionConstraint) const override {
        return resolutionConstraint == this;
    }

    void eachReificationTDR(const std::function<void (TypeDefinitionReification&)> &callback) override {
        return eachReification(callback);
    }
};

}  // namespace EmojicodeCompiler

#endif /* Protocol_hpp */
