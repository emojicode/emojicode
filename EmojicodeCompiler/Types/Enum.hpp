//
//  Enum.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Enum_hpp
#define Enum_hpp

#include "../EmojicodeCompiler.hpp"
#include "ValueType.hpp"
#include <map>
#include <utility>

namespace EmojicodeCompiler {

class Enum : public ValueType {
public:
    Enum(std::u32string name, Package *package, SourcePosition position, const std::u32string &documentation)
        : ValueType(std::move(name), package, std::move(position), documentation) {
            makePrimitive();
    }

    std::pair<bool, long> getValueFor(const std::u32string &c) const;
    void addValueFor(const std::u32string &c, const SourcePosition &position, std::u32string documentation);

    const std::map<std::u32string, std::pair<long, std::u32string>>& values() const { return map_; }

    bool canBeUsedToResolve(TypeDefinition *resolutionConstraint) const override { return false; }
private:
    std::map<std::u32string, std::pair<long, std::u32string>> map_;
    long nextValue_ = 0;
};

}  // namespace EmojicodeCompiler

#endif /* Enum_hpp */
