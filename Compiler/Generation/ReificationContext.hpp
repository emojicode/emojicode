//
// Created by Theo Weidmann on 25.12.17.
//

#ifndef EMOJICODE_REIFICATIONCONTEXT_HPP
#define EMOJICODE_REIFICATIONCONTEXT_HPP

#include "Types/Type.hpp"
#include <map>

namespace EmojicodeCompiler {

struct TypeDefinitionReification;
template <typename Entity>
struct Reification;

class ReificationContext {
public:
    explicit ReificationContext(const std::map<size_t, Type> &localGenericArgs,
                                const Reification<TypeDefinitionReification> *ownerReification)
            : localGenericArguments_(localGenericArgs), ownerReification_(ownerReification) {}
    const Type& actualType(size_t index) const {
        return localGenericArguments_.find(index)->second;
    }
    bool providesActualTypeFor(size_t index) const {
        return localGenericArguments_.find(index) != localGenericArguments_.end();
    }

    const std::map<size_t, Type>& arguments() const { return localGenericArguments_; }

    const Reification<TypeDefinitionReification>* ownerReification() const { return ownerReification_; }

private:
    const std::map<size_t, Type> &localGenericArguments_;
    const Reification<TypeDefinitionReification> *ownerReification_;
};

}  // namespace EmojicodeCompiler

#endif //EMOJICODE_REIFICATIONCONTEXT_HPP
