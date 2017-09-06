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

class Protocol : public TypeDefinition {
public:
    Protocol(std::u32string name, Package *pkg, const SourcePosition &p, const std::u32string &string);

    void prepareForSemanticAnalysis() override;

    bool canBeUsedToResolve(TypeDefinition *resolutionConstraint) const override {
        return resolutionConstraint == this;
    }
};

}  // namespace EmojicodeCompiler

#endif /* Protocol_hpp */
