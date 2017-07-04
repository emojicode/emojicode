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

namespace EmojicodeCompiler {

class ScoperWithScope : public Scoper {
public:
    ScoperWithScope() {}
    ScoperWithScope& operator=(ScoperWithScope &other) {
        if (this != &other) {
            Scoper::operator=(other);
            scope_ = other.scope_;
            scope_.setScoper(this);
        }
        return *this;
    }
    ScoperWithScope(ScoperWithScope &sws) : scope_(sws.scope_) {
        scope_.setScoper(this);
    }
    Scope& scope() { return scope_; }
private:
    Scope scope_ = Scope(this);
};

};  // namespace EmojicodeCompiler

#endif /* ScoperWithScope_hpp */
