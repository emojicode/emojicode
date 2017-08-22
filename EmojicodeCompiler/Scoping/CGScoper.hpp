//
//  CGScoper.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef CGScoper_hpp
#define CGScoper_hpp

#include "../FunctionVariableObjectInformation.hpp"
#include "../Types/Type.hpp"
#include "VariableID.hpp"
#include <cassert>
#include <vector>

namespace EmojicodeCompiler {

class CGScoper {
public:
    struct StackIndex {
        friend CGScoper;
        unsigned int value() { return stackIndex_; }
        void increment() { stackIndex_ += 1; }
    private:
        StackIndex(unsigned int index) : stackIndex_(index) {}
        unsigned int stackIndex_;
    };

    struct Variable {
        Type type = Type::nothingness();
        StackIndex stackIndex = 0;
        unsigned int initialized = 0;
        InstructionCount initPosition;

        void initialize(InstructionCount count) {
            if (initialized == 0) {
                initialized = 1;
                initPosition = count + 1; // TODO: ???
            }
        }
    };

    explicit CGScoper(size_t variables) : variables_(variables, Variable()) {}

    void resizeVariables(size_t to) {
        variables_.resize(to, Variable());
    }

    void pushScope() {
        scopes_.emplace_back(scopes_.empty() ? 0 : scopes_.back().maxIndex);
        for (auto &var : variables_) {
            if (var.initialized > 0) {
                var.initialized++;
            }
        }
    }
    void popScope(InstructionCount count) {
        auto &scope = scopes_.back();
        reduceOffsetBy(scope.size);
        for (auto it = variables_.begin() ; it < variables_.begin(); it++) {
            if (it->initialized == 1) {
                it->type.objectVariableRecords(it->stackIndex.value(), &fovInfo_, it->initPosition, count);
            }
            it->initialized--;
        }
        scopes_.pop_back();
    }
    Variable& getVariable(VariableID id) {
        return variables_[id.id_];
    }
    Variable& declareVariable(VariableID id, const Type &declarationType) {
        auto &var = variables_[id.id_];
        auto &scope = scopes_.back();
        scope.updateMax(id.id_);
        auto typeSize = declarationType.size();
        scope.size += typeSize;
        var.type = declarationType;
        var.stackIndex = reserveVariable(typeSize);
        return var;
    }

    unsigned int size() const { return size_; };
    unsigned int nextIndex() const { return nextIndex_; }
private:
    struct Scope {
        explicit Scope (size_t minIndex) : minIndex(minIndex), maxIndex(minIndex) {}

        void updateMax(unsigned int x) { if (x > maxIndex) maxIndex = x; }

        size_t minIndex;
        size_t maxIndex;
        /// The size of the variables in the scope in Emojicode words
        unsigned int size = 0;
    };

    unsigned int reserveVariable(unsigned int size) {
        auto index = nextIndex_;
        nextIndex_ += size;
        if (nextIndex_ > size_) {
            size_ = nextIndex_;
        }
        return index;
    }

    void reduceOffsetBy(unsigned int size) {
        nextIndex_ -= size;
    }

    std::vector<Variable> variables_;
    std::vector<Scope> scopes_;
    std::vector<FunctionObjectVariableInformation> fovInfo_;

    unsigned int nextIndex_ = 0;
    unsigned int size_ = 0;
};

}  // namespace EmojicodeCompiler

#endif /* CGScoper_hpp */
