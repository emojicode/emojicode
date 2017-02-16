//
//  Scoper.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 14/11/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Scoper_hpp
#define Scoper_hpp

#include "Scope.hpp"

class Scoper {
    friend Scope;
public:
    /*
     * Returns the size of this frame at full use.
     * @warning Beware of that it is possible to reserve slots without setting a variable. The value returned
     *          is therefore not always equal to the variables in a scope.
     */
    int fullSize() const { return size_; };
protected:
    void reduceOffsetBy(int size);
private:
    int nextOffset_ = 0;
    int size_ = 0;

    void syncSize();

    int reserveVariable(int size);
};

#endif /* Scoper_hpp */
