//
//  Protocol.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Protocol_hpp
#define Protocol_hpp

#include "../Package/Package.hpp"
#include "TypeContext.hpp"
#include "TypeDefinition.hpp"
#include <map>
#include <vector>

namespace EmojicodeCompiler {

class Protocol : public TypeDefinition {
public:
    Protocol(std::u32string name, Package *pkg, const SourcePosition &p, const std::u32string &string);

    uint_fast16_t index;

    bool canBeUsedToResolve(TypeDefinition *resolutionConstraint) const override {
        return resolutionConstraint == this;
    }
    void addMethod(Function *method) override;

    int size() const override { return 1; }
private:
    VTIProvider* protocolMethodVtiProvider() override { return nullptr; }
};

}  // namespace EmojicodeCompiler

#endif /* Protocol_hpp */
