//
//  Protocol.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Protocol_hpp
#define Protocol_hpp

#include <vector>
#include <map>
#include "Package.hpp"
#include "TypeDefinitionFunctional.hpp"
#include "TypeContext.hpp"

namespace EmojicodeCompiler {

class Protocol : public TypeDefinitionFunctional {
public:
    Protocol(EmojicodeString name, Package *pkg, SourcePosition p, const EmojicodeString &string);

    uint_fast16_t index;

    bool canBeUsedToResolve(TypeDefinitionFunctional *resolutionConstraint) const override {
        return resolutionConstraint == this;
    }
    void addMethod(Function *method) override;

    int size() const override { return 1; }
private:
    static uint_fast16_t nextIndex;
};

}  // namespace EmojicodeCompiler

#endif /* Protocol_hpp */
