//
//  Enum.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Enum_hpp
#define Enum_hpp

#include <map>
#include <utility>
#include "../EmojicodeCompiler.hpp"
#include "ValueType.hpp"

namespace EmojicodeCompiler {

class Enum : public ValueType {
public:
    Enum(EmojicodeString name, Package *package, SourcePosition position, const EmojicodeString &documentation)
        : ValueType(name, package, position, documentation) {
            makePrimitive();
    }

    std::pair<bool, long> getValueFor(const EmojicodeString &c) const;
    void addValueFor(const EmojicodeString &c, const SourcePosition &position, EmojicodeString documentation);

    const std::map<EmojicodeString, std::pair<long, EmojicodeString>>& values() const { return map_; }

    int size() const override { return 1; }

    bool canBeUsedToResolve(TypeDefinition *resolutionConstraint) const override { return false; }
private:
    std::map<EmojicodeString, std::pair<long, EmojicodeString>> map_;
    long valuesCounter = 0;
};

}  // namespace EmojicodeCompiler

#endif /* Enum_hpp */
