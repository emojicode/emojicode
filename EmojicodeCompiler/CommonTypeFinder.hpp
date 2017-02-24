//
//  CommonTypeFinder.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CommonTypeFinder_hpp
#define CommonTypeFinder_hpp

#include <vector>
#include "Type.hpp"
#include "Token.hpp"

class CommonTypeFinder {
public:
    /// Tells the common type finder about the type of another element in the collection.
    void addType(Type t, TypeContext typeContext);
    /** Returns the common type and issues a warning at @c warningToken if the common type is ambigious. */
    Type getCommonType(SourcePosition p) const;
private:
    bool firstTypeFound_ = false;
    Type commonType_ = Type::something();
    std::vector<Type> commonProtocols_;
};


#endif /* CommonTypeFinder_hpp */
