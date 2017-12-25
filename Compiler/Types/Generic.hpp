//
//  Generic.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 26/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Generic_hpp
#define Generic_hpp

#include "CompilerError.hpp"
#include "Type.hpp"
#include <map>
#include <string>
#include <vector>

namespace EmojicodeCompiler {

struct SourcePosition;

struct GenericParameter {
    GenericParameter(std::u32string name, Type constraint, bool rejectsBoxing)
            : name(std::move(name)), constraint(std::move(constraint)), rejectsBoxing(rejectsBoxing) {}
    std::u32string name;
    Type constraint;
    bool rejectsBoxing;
};

template <typename T>
class Generic {
public:
    /**
     * Tries to fetch the type reference type for the given generic variable name and stores it into @c type.
     * @returns Whether the variable could be found or not. @c type is untouched if @c false was returned.
     */
    bool fetchVariable(const std::u32string &name, bool optional, Type *destType) {
        auto it = parameterVariables_.find(name);
        if (it != parameterVariables_.end()) {
            Type type = Type(false, it->second, static_cast<T *>(this), !genericParameters_[it->second].rejectsBoxing);
            if (optional) {
                type.setOptional();
            }
            *destType = type;
            return true;
        }
        return false;
    }


    bool requiresReificationCopy() const {
        return std::any_of(genericParameters_.begin(), genericParameters_.end(), [](const GenericParameter &param) {
            return param.rejectsBoxing;
        });
    }

    /// Adds a new generic argument to the end of the list.
    /// @param variableName The name which is used to refer to this argument.
    /// @param constraint The constraint that applies for the argument.
    /// @param rejectsBoxing If this argument is true, no boxing will be performed when treating a compatible value as
    ///                      an instance of the type of this generic parameter. The storage type will always be
    ///                      StorageType::Simple or StorageType::SimpleOptional.
    void addGenericParameter(const std::u32string &variableName, const Type &constraint, bool rejectsBoxing,
                             const SourcePosition &p) {
        genericParameters_.emplace_back(variableName, constraint, rejectsBoxing);

        if (parameterVariables_.find(variableName) != parameterVariables_.end()) {
            throw CompilerError(p, "A generic argument variable with the same name is already in use.");
        }
        parameterVariables_.emplace(variableName, parameterVariables_.size());
    }

    /// Returns the type constraint of the generic parameter at the index.
    /// @pre The index must be greater than the offset passed to offsetIndicesBy() (e.g. the number of super generic
    /// arguments for a TypeDefinition) if applicable.
    const Type& constraintForIndex(size_t index) const {
        return genericParameters_[index - offset_].constraint;
    }
    /// Returns the name of the generic parameter at the index.
    /// @pre The index must be greater than the offset passed to offsetIndicesBy() (e.g. the number of super generic
    /// arguments for a TypeDefinition) if applicable.
    const std::u32string& findGenericName(size_t index) const {
        return genericParameters_[index - offset_].name;
    }

    /// The number of generic parameters defined.
    size_t genericParameterCount() const { return genericParameters_.size(); }

    /// @returns A vector with pairs of std::u32string and Type. These represent the generic parameter name and the
    /// the type constraint.
    const std::vector<GenericParameter>& parameters() const { return genericParameters_; }
protected:
    /// Offsets the genericVariableIndex() of the Type instances returned by fetchVariable() by offset.
    void offsetIndicesBy(size_t offset) {
        offset_ = offset;
        for (auto &param : parameterVariables_) {
            param.second += offset;
        }
    }
private:
    /// The properties of the generic parameters. Their position in the vector matches their index minus
    /// the number of super type generic parameters if applicable (see TypeDefinition).
    std::vector<GenericParameter> genericParameters_;
    /** Generic type arguments as variables */
    std::map<std::u32string, size_t> parameterVariables_;

    size_t offset_ = 0;
};

} // namespace EmojicodeCompiler


#endif /* Generic_hpp */
