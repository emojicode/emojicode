//
//  ArgumentsMigCreator.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ArgumentsMigCreator_hpp
#define ArgumentsMigCreator_hpp

#include <vector>
#include <cstdlib>

namespace EmojicodeCompiler {

class Function;
class MigWriter;

class ArgumentsMigCreator {
    friend MigWriter;
public:
    class ArgumentsExpector {
        friend ArgumentsMigCreator;
    public:
        void provide(unsigned int count) {
            creator_->arguments_[index_] = count;
        }
    private:
        ArgumentsExpector(size_t index, ArgumentsMigCreator *creator) : index_(index), creator_(creator) {}
        size_t index_;
        ArgumentsMigCreator *creator_;
    };
    friend ArgumentsExpector;

    ArgumentsExpector takesArguments() {
        auto index = arguments_.size();
        arguments_.emplace_back(0);
        return ArgumentsExpector(index, this);
    }
private:
    std::vector<unsigned int> arguments_;
};

}

#endif /* ArgumentsMigCreator_hpp */
