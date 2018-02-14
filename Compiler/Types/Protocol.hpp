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
    Protocol(std::u32string name, Package *pkg, const SourcePosition &p, const std::u32string &string, bool exported);

    size_t index() { return index_; }
    void setIndex(size_t index) { index_ = index; }

    bool canBeUsedToResolve(TypeDefinition *resolutionConstraint) const override {
        return resolutionConstraint == this;
    }
private:
    size_t index_;
};

}  // namespace EmojicodeCompiler

#endif /* Protocol_hpp */
