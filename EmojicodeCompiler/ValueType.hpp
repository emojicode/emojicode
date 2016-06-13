//
//  ValueType.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 12/06/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef ValueType_hpp
#define ValueType_hpp

#include "TypeDefinitionFunctional.hpp"

class ValueType : public TypeDefinitionFunctional {
public:
    ValueType(EmojicodeChar name, Package *p, SourcePosition pos, const EmojicodeString &documentation)
        : TypeDefinitionFunctional(name, p, pos, documentation) {}
    
    bool canBeUsedToResolve(TypeDefinitionFunctional *resolutionConstraint) { return false; }
    
    virtual void addMethod(Method *method);
    virtual void addInitializer(Initializer *method);
    virtual void addClassMethod(ClassMethod *method);
};

#endif /* ValueType_hpp */
