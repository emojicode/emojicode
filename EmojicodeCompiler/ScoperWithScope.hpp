//
//  ScoperWithScope.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 14/11/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef ScoperWithScope_hpp
#define ScoperWithScope_hpp

#include "Scoper.hpp"

class ScoperWithScope : public Scoper {
public:
    Scope& scope() { return scope_; }
private:
    Scope scope_ = Scope(this);
};

#endif /* ScoperWithScope_hpp */
