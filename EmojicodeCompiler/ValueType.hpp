//
//  ValueType.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 12/06/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef ValueType_hpp
#define ValueType_hpp

#include <vector>
#include "TypeDefinitionFunctional.hpp"

class ValueType : public TypeDefinitionFunctional {
public:
    static const std::vector<ValueType *>& valueTypes() { return valueType_; }
    
    ValueType(EmojicodeChar name, Package *p, SourcePosition pos, const EmojicodeString &documentation)
        : TypeDefinitionFunctional(name, p, pos, documentation) {
            valueType_.push_back(this);
    }
    
    bool canBeUsedToResolve(TypeDefinitionFunctional *resolutionConstraint) { return false; }
private:
    static std::vector<ValueType *> valueType_;
};

#endif /* ValueType_hpp */
