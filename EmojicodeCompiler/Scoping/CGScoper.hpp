//
//  CGScoper.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef CGScoper_hpp
#define CGScoper_hpp

#include <vector>

namespace EmojicodeCompiler {

template <typename T>
class CGScoper {
public:
    explicit CGScoper(size_t variables) : variables_(variables, T()) {}

    void resizeVariables(size_t to) {
        variables_.resize(to, T());
    }

    T& getVariable(size_t id) {
        return variables_[id];
    }
private:
    std::vector<T> variables_;
};

}  // namespace EmojicodeCompiler

#endif /* CGScoper_hpp */
