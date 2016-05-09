//
//  TypeContext.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef TypeContext_hpp
#define TypeContext_hpp

#include "Type.hpp"
#include "CommonTypeFinder.hpp"
#include "Procedure.hpp"

class TypeContext {
public:
    TypeContext(Type callee) : calleeType_(callee) {};
    TypeContext(Type callee, Procedure *p) : calleeType_(callee), procedure_(p) {};
    TypeContext(Type callee, Procedure *p, std::vector<Type> *args)
        : calleeType_(callee), procedure_(p), procedureGenericArguments_(args) {};
    
    Type calleeType() const { return calleeType_; }
    Procedure* procedure() const { return procedure_; }
    std::vector<Type>* procedureGenericArguments() const { return procedureGenericArguments_; }
private:
    Type calleeType_;
    Procedure *procedure_ = nullptr;
    std::vector<Type> *procedureGenericArguments_ = nullptr;
};

#endif /* TypeContext_hpp */
