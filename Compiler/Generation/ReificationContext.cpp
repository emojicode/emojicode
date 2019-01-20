//
// Created by Theo Weidmann on 25.12.17.
//

#include "ReificationContext.hpp"
#include "Types/TypeDefinition.hpp"

namespace EmojicodeCompiler {

const Type* ReificationContext::actualType(const Type &type) const {
    if (type.type() == TypeType::LocalGenericVariable) {
        auto it = localGenericArguments_.find(type.genericVariableIndex());
        return it != localGenericArguments_.end() ? &it->second : nullptr;
    }
    if (type.type() == TypeType::GenericVariable && ownerReification_ != nullptr) {
        auto it = ownerReification_->arguments.find(type.genericVariableIndex());
        return it != ownerReification_->arguments.end() ? &it->second : nullptr;
    }
    return nullptr;
}

}  // namespace EmojicodeCompiler
