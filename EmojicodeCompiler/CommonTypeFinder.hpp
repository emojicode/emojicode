//
//  CommonTypeFinder.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CommonTypeFinder_hpp
#define CommonTypeFinder_hpp

#include "Type.hpp"
#include "Token.hpp"

struct CommonTypeFinder {
    /** Tells the common type finder about the type of another element in the collection/data structure. */
    void addType(Type t, TypeContext typeContext);
    /** Returns the common type and issues a warning at @c warningToken if the common type is ambigious. */
    Type getCommonType(const Token &warningToken) const;
private:
    bool firstTypeFound = false;
    Type commonType = typeSomething;
};


#endif /* CommonTypeFinder_hpp */
