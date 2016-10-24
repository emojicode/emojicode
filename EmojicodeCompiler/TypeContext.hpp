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

class Function;

class TypeContext {
public:
    TypeContext(Type callee) : calleeType_(callee) {};
    TypeContext(Type callee, Function *p) : calleeType_(callee), function_(p) {};
    TypeContext(Type callee, Function *p, std::vector<Type> *args)
        : calleeType_(callee), function_(p), functionGenericArguments_(args) {};
    
    Type calleeType() const { return calleeType_; }
    Function* function() const { return function_; }
    std::vector<Type>* functionGenericArguments() const { return functionGenericArguments_; }
private:
    Type calleeType_;
    Function *function_ = nullptr;
    std::vector<Type> *functionGenericArguments_ = nullptr;
};

#endif /* TypeContext_hpp */
