//
// Created by Theo Weidmann on 25.12.17.
//

#ifndef EMOJICODE_REIFICATIONCONTEXT_HPP
#define EMOJICODE_REIFICATIONCONTEXT_HPP

#include "Types/Type.hpp"
#include <map>

namespace EmojicodeCompiler {

struct TypeDefinitionReification;

class ReificationContext {
public:
    explicit ReificationContext(std::map<size_t, Type> localGenericArgs,
                                const TypeDefinitionReification *ownerReification)
            : localGenericArguments_(std::move(localGenericArgs)), ownerReification_(ownerReification) {}

    const Type* actualType(const Type &type) const;

    const std::map<size_t, Type>& arguments() const { return localGenericArguments_; }

    const TypeDefinitionReification* ownerReification() const { return ownerReification_; }

private:
    const std::map<size_t, Type> localGenericArguments_;
    const TypeDefinitionReification *ownerReification_;
};

}  // namespace EmojicodeCompiler

#endif //EMOJICODE_REIFICATIONCONTEXT_HPP
