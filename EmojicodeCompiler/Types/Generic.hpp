//
//  Generic.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 26/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Generic_hpp
#define Generic_hpp

#include "../CompilerError.hpp"
#include "Type.hpp"
#include <string>
#include <vector>
#include <map>

namespace EmojicodeCompiler {

struct SourcePosition;

template <typename T>
class Generic {
public:
    /**
     * Tries to fetch the type reference type for the given generic variable name and stores it into @c type.
     * @returns Whether the variable could be found or not. @c type is untouched if @c false was returned.
     */
    bool fetchVariable(const std::u32string &name, bool optional, Type *destType) {
        auto it = genericArgumentVariables_.find(name);
        if (it != genericArgumentVariables_.end()) {
            Type type = Type(false, it->second, static_cast<T *>(this));
            if (optional) {
                type.setOptional();
            }
            *destType = type;
            return true;
        }
        return false;
    }

    /// Adds a new generic argument to the end of the list.
    /// @param variableName The name which is used to refer to this argument.
    /// @param constraint The constraint that applies for the argument.
    void addGenericArgument(const std::u32string &variableName, const Type &constraint, const SourcePosition &p) {
        genericArgumentConstraints_.emplace_back(variableName, constraint);

        if (genericArgumentVariables_.find(variableName) != genericArgumentVariables_.end()) {
            throw CompilerError(p, "A generic argument variable with the same name is already in use.");
        }
        genericArgumentVariables_.emplace(variableName, genericArgumentVariables_.size());
    }

    /// Returns the type constraint of the generic parameter at the index.
    /// @pre The index must be greater than the offset passed to offsetIndicesBy() (e.g. the number of super generic
    /// arguments for a TypeDefinition) if applicable.
    const Type& constraintForIndex(size_t index) const {
        return genericArgumentConstraints_[index - offset_].second;
    }
    /// Returns the name of the generic parameter at the index.
    /// @pre The index must be greater than the offset passed to offsetIndicesBy() (e.g. the number of super generic
    /// arguments for a TypeDefinition) if applicable.
    const std::u32string& findGenericName(size_t index) const {
        return genericArgumentConstraints_[index - offset_].first;
    }

    /// The number of generic parameters defined.
    size_t genericParameterCount() const { return genericArgumentConstraints_.size(); }

    /// @returns A vector with pairs of std::u32string and Type. These represent the generic parameter name and the
    /// the type constraint.
    const std::vector<std::pair<std::u32string, Type>>& parameters() const { return genericArgumentConstraints_; }
protected:
    /// Offsets the genericVariableIndex() of the Type instances returned by fetchVariable() by offset.
    void offsetIndicesBy(size_t offset) {
        offset_ = offset;
        for (auto &param : genericArgumentVariables_) {
            param.second += offset;
        }
    }
private:
    /// The constraints for the generic parameter. Their position in the vector matches their index minus
    /// the number of super type generic parameters if applicable (see TypeDefinition).
    std::vector<std::pair<std::u32string, Type>> genericArgumentConstraints_;
    /** Generic type arguments as variables */
    std::map<std::u32string, size_t> genericArgumentVariables_;

    size_t offset_ = 0;
};

} // namespace EmojicodeCompiler


#endif /* Generic_hpp */
