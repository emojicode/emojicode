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
#include "EmojicodeCompiler.hpp"
#include "TypeDefinition.hpp"

class Enum : public TypeDefinition {
public:
    Enum(EmojicodeString name, Package *package, SourcePosition position, const EmojicodeString &documentation)
        : TypeDefinition(name, package, position, documentation) {}

    std::pair<bool, long> getValueFor(EmojicodeString c) const;
    void addValueFor(EmojicodeString c, SourcePosition position, EmojicodeString documentation);

    const std::map<EmojicodeString, std::pair<long, EmojicodeString>>& values() const { return map_; }

    int size() const override { return 1; }
private:
    std::map<EmojicodeString, std::pair<long, EmojicodeString>> map_;
    long valuesCounter = 0;
};

#endif /* Enum_hpp */
