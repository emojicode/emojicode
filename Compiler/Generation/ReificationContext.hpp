//
// Created by Theo Weidmann on 25.12.17.
//

#ifndef EMOJICODE_REIFICATIONCONTEXT_HPP
#define EMOJICODE_REIFICATIONCONTEXT_HPP

#include "Types/Generic.hpp"
#include "Types/Type.hpp"
#include <map>

namespace EmojicodeCompiler {

class ReificationContext {
public:
    template <typename T, typename E>
    explicit ReificationContext(const Generic<T, E> &, const typename Generic<T, E>::Reification &reification)
            : copyReificationRequest_(reification.arguments) {}
    const Type& actualType(size_t index) const {
        return copyReificationRequest_.find(index)->second;
    }
    bool providesActualTypeFor(size_t index) const {
        return copyReificationRequest_.find(index) != copyReificationRequest_.end();
    }

private:
    const std::map<size_t, Type> &copyReificationRequest_;
};

};  // namespace EmojicodeCompiler

#endif //EMOJICODE_REIFICATIONCONTEXT_HPP
