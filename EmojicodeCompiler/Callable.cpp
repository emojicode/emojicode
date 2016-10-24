//
//  Callable.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/09/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Callable.hpp"

Type Callable::type() const {
    Type t(TypeContent::Callable, false);
    t.genericArguments.push_back(returnType);
    for (int i = 0; i < arguments.size(); i++) {
        t.genericArguments.push_back(arguments[i].type);
    }
    return t;
}
