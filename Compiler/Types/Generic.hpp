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
#include "AST/ASTType.hpp"
#include <algorithm>
#include <cassert>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace EmojicodeCompiler {

struct SourcePosition;

struct GenericParameter {
    GenericParameter(std::u32string name, std::unique_ptr<ASTType> constraint, bool useBox)
            : name(std::move(name)), constraint(std::move(constraint)), useBox(useBox) {}
    std::u32string name;
    std::unique_ptr<ASTType> constraint;
    bool useBox;
};

template <typename Entity>
struct Reification {
    Entity entity;
    std::map<size_t, Type> arguments;
};

template <typename T, typename Entity>
class Generic {
public:
    /**
     * Tries to fetch the type reference type for the given generic variable name and stores it into @c type.
     * @returns Whether the variable could be found or not. @c type is untouched if @c false was returned.
     */
    bool fetchVariable(const std::u32string &name, Type *destType) {
        auto it = parameterVariables_.find(name);
        if (it != parameterVariables_.end()) {
            *destType = typeForVariable(it->second);
            return true;
        }
        return false;
    }

    Type typeForVariable(size_t n) {
        assert(offset_ <= n && n < offset_ + genericParameters_.size());
        Type type = Type(n, static_cast<T *>(this));
        if (genericParameters_[n - offset_].useBox) {
            type = type.boxedFor(constraintForIndex(n));
        }
        return type;
    }

    /// Adds a new generic argument to the end of the list.
    /// @param variableName The name which is used to refer to this argument.
    /// @param constraint The constraint that applies for the argument.
    /// @param rejectsBoxing If this argument is true, no boxing will be performed when treating a compatible value as
    ///                      an instance of the type of this generic parameter. The storage type will always be
    ///                      StorageType::Simple or StorageType::SimpleOptional.
    void addGenericParameter(const std::u32string &variableName, std::unique_ptr<ASTType> constraint,
                             bool rejectsBoxing, const SourcePosition &p) {
        genericParameters_.emplace_back(variableName, std::move(constraint), !rejectsBoxing);

        if (parameterVariables_.find(variableName) != parameterVariables_.end()) {
            throw CompilerError(p, "A generic argument variable with the same name is already in use.");
        }
        parameterVariables_.emplace(variableName, parameterVariables_.size());
    }

    void requestReificationAndCheck(const TypeContext &typeContext, const std::vector<Type> &args,
                                    const SourcePosition &p) {
        if (args.size() - offset_ != genericParameters().size()) {
            throw CompilerError(p, "Expected ", genericParameters().size(), " generic arguments, but ",
                                args.size(), " are provided.");
        }

        for (size_t i = offset_; i < args.size(); i++) {
            if (!args[i].compatibleTo(constraintForIndex(i), typeContext)) {
                throw CompilerError(p, "Generic argument ", i + 1, " of type ",
                                    args[i].toString(typeContext), " is not compatible to constraint ",
                                    constraintForIndex(offset_ + i).toString(typeContext), ".");
            }
        }

        requestReification(args);
    }

    Entity& reificationFor(const std::vector<Type> &arguments) {
        auto ref = reifications_.find(buildKey(arguments));
        assert(ref != reifications_.end());
        return ref->second.entity;
    }

    Reification<Entity>& reificationWrapperFor(const std::vector<Type> &arguments) {
        auto ref = reifications_.find(buildKey(arguments));
        assert(ref != reifications_.end());
        return ref->second;
    }

    /// @returns Any reification of this instance.
    /// @note This method is intended to be used when there is only ever one possible reification of an instance.
    Entity& unspecificReification() {
        assert(reifications_.size() == 1);
        return reifications_.begin()->second.entity;
    }

    /// Creates an unspecific reification if no reification was requested yet.
    /// @note This method asserts that there is only one reification as this method
    /// should only be called in combination with unspecificReification()
    Entity& createUnspecificReification() {
        assert(!requiresCopyReification());
        if (reifications_.empty()) {
            reifications_.emplace();
        }
        return unspecificReification();
    }

    bool requiresCopyReification() const {
        return std::any_of(genericParameters_.begin(), genericParameters_.end(), [](auto &param) {
            return !param.useBox;
        });
    }

    /// Returns the type constraint of the generic parameter at the index.
    /// @pre The index must be greater than the offset passed to offsetIndicesBy() (e.g. the number of super generic
    /// arguments for a TypeDefinition) if applicable.
    const Type& constraintForIndex(size_t index) const {
        return genericParameters_[index - offset_].constraint->type();
    }
    /// Returns the name of the generic parameter at the index.
    /// @pre The index must be greater than the offset passed to offsetIndicesBy() (e.g. the number of super generic
    /// arguments for a TypeDefinition) if applicable.
    const std::u32string& findGenericName(size_t index) const {
        return genericParameters_[index - offset_].name;
    }

    void analyseConstraints(const TypeContext &typeContext) {
        for (auto &param : genericParameters_) {
            auto &type = param.constraint->analyseType(typeContext);
//            if (type.type() == TypeType::Someobject) {
//                param.useBox = false;
//            }
        }
    }

    /// @returns A vector with pairs of std::u32string and Type. These represent the generic parameter name and the
    /// the type constraint.
    const std::vector<GenericParameter>& genericParameters() const { return genericParameters_; }

    void eachReification(const std::function<void (Reification<Entity>&)> &callback) {
        for (auto &reification : reifications_) {
            callback(reification.second);
        }
    }

    const std::map<std::vector<Type>, Reification<Entity>>& reificationMap() const { return reifications_; }

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

    std::map<std::vector<Type>, Reification<Entity>> reifications_;

    size_t offset_ = 0;

    std::vector<Type> buildKey(const std::vector<Type> &arguments) {
        std::vector<Type> key;
        for (size_t i = 0; i < genericParameters_.size(); i++) {
            if (!genericParameters_[i].useBox) {
                key.emplace_back(arguments[i].type() == TypeType::Class ? Type::someobject() : arguments[i]);
            }
        }
        return key;
    }

    void requestReification(const std::vector<Type> &arguments) {
        auto key = buildKey(arguments);
        if (reifications_.find(key) != reifications_.end()) {
            return;
        }
        auto &reification = reifications_[key] = Reification<Entity>();
        for (size_t i = 0; i < genericParameters_.size(); i++) {
            if (!genericParameters_[i].useBox) {
                reification.arguments.emplace(i + offset_, arguments[i]);
            }
        }
    }
};

} // namespace EmojicodeCompiler


#endif /* Generic_hpp */
