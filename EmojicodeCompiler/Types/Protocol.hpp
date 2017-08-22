//
//  Protocol.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Protocol_hpp
#define Protocol_hpp

#include "../Parsing/Package.hpp"
#include "TypeContext.hpp"
#include "TypeDefinition.hpp"
#include <map>
#include <vector>

namespace EmojicodeCompiler {

class Protocol : public TypeDefinition {
public:
    Protocol(EmojicodeString name, Package *pkg, const SourcePosition &p, const EmojicodeString &string);

    uint_fast16_t index;

    bool canBeUsedToResolve(TypeDefinition *resolutionConstraint) const override {
        return resolutionConstraint == this;
    }
    void addMethod(Function *method) override;

    int size() const override { return 1; }
private:
    static uint_fast16_t nextIndex;
    VTIProvider* protocolMethodVtiProvider() override { return nullptr; }
};

}  // namespace EmojicodeCompiler

#endif /* Protocol_hpp */
