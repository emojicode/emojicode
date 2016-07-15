//
//  Enum.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Enum_hpp
#define Enum_hpp

#include "EmojicodeCompiler.hpp"
#include "TypeDefinition.hpp"

class Enum : public TypeDefinition {
public:
    Enum(EmojicodeChar name, Package *package, SourcePosition position, const EmojicodeString &documentation)
        : TypeDefinition(name, package, position, documentation) {}
    
    std::pair<bool, EmojicodeInteger> getValueFor(EmojicodeChar c) const;
    void addValueFor(EmojicodeChar c, SourcePosition position, EmojicodeString documentation);
    
    const std::map<EmojicodeChar, std::pair<EmojicodeInteger, EmojicodeString>>& values() const { return map_; }
private:
    std::map<EmojicodeChar, std::pair<EmojicodeInteger, EmojicodeString>> map_;
    EmojicodeInteger valuesCounter = 0;
};

#endif /* Enum_hpp */
