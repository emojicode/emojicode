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

class Protocol : public TypeDefinitionFunctional {
public:
    Protocol(EmojicodeChar name, Package *pkg, SourcePosition p, const EmojicodeString &string);
    
    uint_fast16_t index;
    
    bool canBeUsedToResolve(TypeDefinitionFunctional *a);
    
    Method* getMethod(const Token &token, Type type, TypeContext typeContext);
    Method* lookupMethod(EmojicodeChar name);
    void addMethod(Method *method);
    const std::vector<Method*>& methods() { return methodList_; };
    
    bool usesSelf() const { return usesSelf_; }
    void setUsesSelf() { usesSelf_ = true; }
private:
    static uint_fast16_t nextIndex;
    
    /** List of all methods. */
    std::vector<Method *> methodList_;
    
    bool usesSelf_ = false;
    
    /** Hashmap holding methods. @warning Don't access it directly, use the correct functions. */
    std::map<EmojicodeChar, Method*> methods_;
};

#endif /* Protocol_hpp */
