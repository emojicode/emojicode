//
//  Type.h
//  Emojicode
//
//  Created by Theo Weidmann on 29/12/15.
//  Copyright © 2015 Theo Weidmann. All rights reserved.
//

#ifndef Type_hpp
#define Type_hpp

#include "StorageType.hpp"
#include <stdexcept>
#include <string>
#include <vector>
#include <tuple>
#include <cassert>

namespace EmojicodeCompiler {

/// The identifier value representing the default namespace.
extern const std::u32string kDefaultNamespace;

class TypeDefinition;
class Enum;
class Class;
class Protocol;
class Package;
class TypeDefinition;
class TypeContext;
class ValueType;
class Function;
class CommonTypeFinder;
class AbstractParser;
class Initializer;

enum class TypeType {
    Box,
    Optional,
    Class,
    MultiProtocol,
    Protocol,
    Enum,
    ValueType,
    NoReturn,
    /** Maybe everything. */
    Something,
    /** Any Object */
    Someobject,
    /** Used with generics */
    GenericVariable,
    LocalGenericVariable,
    Callable,
    TypeAsValue,
    Error,
    StorageExpectation,
};

struct MakeTypeAsValueType {};
constexpr MakeTypeAsValueType MakeTypeAsValue {};

/// Represents the type of variable, an argument or the return value of a Function such as a method, an initializer,
/// or type method.
class Type {
public:
    Type(MakeTypeAsValueType makeTypeAsValue, Type type)
            : typeContent_(TypeType::TypeAsValue), genericArguments_({ std::move(type) }) {}

    explicit Type(Class *klass);
    explicit Type(Protocol *protocol);
    explicit Type(Enum *enumeration);
    explicit Type(ValueType *valueType);
    /// Creates a TypeType::Callable type corresponding to the parameters and return type of the provided function.
    explicit Type(Function *function);
    /// Creates a MultiProtocol type.
    /// @param protocols Vector of protocols.
    explicit Type(std::vector<Type> protocols);
    /// Creates a callable type.
    explicit Type(Type returnType, const std::vector<Type> &params)
            : typeContent_(TypeType::Callable), genericArguments_({ std::move(returnType) }) {
        genericArguments_.insert(genericArguments_.end(), params.begin(), params.end());
    }

    /// Creates a generic variable to the generic argument @c r.
    Type(size_t r, TypeDefinition *resolutionConstraint)
            : typeContent_(TypeType::GenericVariable), genericArgumentIndex_(r),
              typeDefinition_(resolutionConstraint) {}

    /// Creates a local generic variable (generic function) to the generic argument @c r.
    Type(size_t r, Function *function)
            : typeContent_(TypeType::LocalGenericVariable), genericArgumentIndex_(r),
              localResolutionConstraint_(function) {}

    static Type something() { return Type(TypeType::Something); }
    static Type noReturn() { return Type(TypeType::NoReturn); }
    static Type someobject() { return Type(TypeType::Someobject); }

    /// @returns The type of this type, i.e. Protocol, Class instance etc.
    TypeType type() const { return typeContent_; }

    /// If this is a box, proxies to the boxed type.
    /// @pre type() == TypeType::Class
    Class* klass() const;
    /// If this is a box, proxies to the boxed type.
    /// @pre type() == TypeType::Protocol
    Protocol* protocol() const;
    /// If this is a box, proxies to the boxed type.
    /// @pre type() == TypeType::Enum
    Enum* enumeration() const;
    /// If this is a box, proxies to the boxed type.
    /// @pre type() == TypeType::ValueType
    ValueType* valueType() const;
    /// If this is a box, proxies to the boxed type.
    /// @pre The represented type must be defined by a TypeDefinition.
    TypeDefinition* typeDefinition() const;

    /// Returns the storage type that will be used, i.e. the boxing applied in memory.
    StorageType storageType() const;

    /// Returns the type boxed in this boxed if this is a Box, otherwise the type itself.
    Type unboxed() const;
    /// Returns type() if this is not a Box otherwise unboxedType().
    TypeType unboxedType() const;
    /// Boxes this type and returns the Box type.
    /// @param forType The type to which compatibility should be established by boxing.
    Type boxedFor(const Type &forType) const;

    Type applyMinimalBoxing() const;

    /// @returns The type to which this box provides compatibility.
    const Type& boxedFor() const {
        assert(type() == TypeType::Box);
        return genericArguments_[1];
    }

    /// @returns True if both types are boxes and are boxed for identical types as determined by identicalTo().
    bool areMatchingBoxes(const Type &type, const TypeContext &context) const;

    /// If this is a box, proxies to the Box and returns the type of the optional in an equal Box.
    /// @returns The type this optional contains. If this type is force boxed, so will be the returned type.
    Type optionalType() const {
        if (type() == TypeType::Box) {
            return genericArguments_[0].optionalType().boxedFor(boxedFor());
        }

        assert(type() == TypeType::Optional);
        return genericArguments_[0];
    }
    /// @returns The type itself if type() does not return TypeType::Optional or optionalType() if it does.
    Type unoptionalized() const { return type() == TypeType::Optional ? optionalType() : *this; }
    /// @returns The type in an optional. If this is a Box, returns an equal Box that contains an optional with the
    /// type inside the box. If this is an optional already, the method returns this instance unchanged.
    Type optionalized() const;
    /// Behaves as optionalized() if true is passed, otherwise returns this instance unchanged.
    Type optionalized(bool optionalize) const { return optionalize ? optionalized() : *this; }

    /// If this is a box, proxies to the Box and returns the type of the optional in an equal Box.
    /// @returns The type (the class, value type etc.) this Type Value Type represents.
    Type typeOfTypeValue() const;

    /// If this is a box, proxies to the Box and returns the type of the optional in an equal Box.
    Type errorType() const;

    Type errored(const Type &errorEnum) const;

    /// If this is a box, proxies to the Box and returns the type of the optional in an equal Box.
    Type errorEnum() const {
        if (type() == TypeType::Box) {
            return genericArguments_[0].errorEnum().boxedFor(boxedFor());
        }

        assert(type() == TypeType::Error);
        return genericArguments_[0];
    }

    /// Returns true if this type is compatible to the given other type.
    bool compatibleTo(const Type &to, const TypeContext &tc, std::vector<CommonTypeFinder> *ctargs = nullptr) const;
    /// Whether this instance of Type is considered indentical to the other instance.
    /// Mainly used to determine compatibility of generics.
    bool identicalTo(Type to, const TypeContext &tc, std::vector<CommonTypeFinder> *ctargs) const;

    /// Returns the generic variable index if the type is a Type::GenericVariable or TypeType::LocalGenericVariable.
    size_t genericVariableIndex() const;

    /// Returns the generic arguments with which this type was specialized.
    const std::vector<Type>& genericArguments() const {
        if (type() == TypeType::Box) {
            return genericArguments_[0].genericArguments_;
        }
        assert(canHaveGenericArguments() || TypeType::Callable == type());
        return genericArguments_;
    }
    /// Allows to change a specific generic argument. @c index must be smaller than @c genericArguments().size()
    void setGenericArgument(size_t index, Type value) { genericArguments_[index] = std::move(value); }
    /// Replaces the generic arguments of this type.
    void setGenericArguments(std::vector<Type> args) {
        if (type() == TypeType::Box) {
            genericArguments_[0].setGenericArguments(std::move(args));
        }
        else {
            assert(args.empty() || canHaveGenericArguments() || TypeType::Callable == type());
            genericArguments_ = args;
        }
    }

    /// True if this type could have generic arguments.
    bool canHaveGenericArguments() const;
    /// True if this type could conform to a protocol.
    bool canHaveProtocol() const;

    /// If this is a box, proxies to the boxed type.
    /// @returns the protocol types of a MultiProtocol
    const std::vector<Type>& protocols() const {
        if (type() == TypeType::Box) {
            return genericArguments_[0].protocols();
        }
        assert(type() == TypeType::MultiProtocol); return genericArguments_;
    }

    /// Tries to resolve this type to a non-generic-variable type by using the generic arguments provided in the
    /// TypeContext. This method also tries to resolve generic arguments to non-generic-variable types recursively.
    /// This method can resolve @c Self, @c References and @c LocalReferences. @c GenericVariable will only be resolved
    /// if the TypeContext’s @c calleeType implementation of @c canResolve returns true for the resolution
    /// constraint, thus only if the generic variable is inteded to be resolved on the given type.
    Type resolveOn(const TypeContext &typeContext) const;
    /**
     * Used to get as mutch information about a reference type as possible without using the generic arguments of
     * the type context’s callee.
     * This method is intended to be used to determine type compatibility while e.g. compiling generic classes.
     */
    Type resolveOnSuperArgumentsAndConstraints(const TypeContext &typeContext) const;

    /// Returns the name of the package to which this type belongs.
    std::string typePackage() const;
    /// Returns a string representation of this type.
    /// @param typeContext The type context to be used when resolving generic argument names. TypeContext() can be
    ///                    provided if the actual Type Context is not known.
    /// @param package If this argument is not nullptr types will be printed with namespace accessors as required for
    ///                code in the provided package. If nullptr, the names of the packages to which types belong are
    ///                printed instead.
    std::string toString(const TypeContext &typeContext, Package *package = nullptr) const;

    std::string namespaceAccessor(Package *package) const;

    /// Returns true if this represents a reference to (a) value(s) of the type represented by this instance.
    /// @see isReferencable()
    bool isReference() const { return isReference_; }

    void setReference(bool v = true) { isReference_ = v; }

    /// Returns true iff a reference to a value of this type is useful. For instance, a reference to an object pointer
    /// is not at all useful. On the other hand, references to value types are required for method calls. Moreover,
    /// boxes can be referenced.
    ///
    /// If this function returns false, this indicates that the value should be dereferenced as soon as possible. This
    /// does not indicate that references to this type will never occur or are forbidden.
    bool isReferenceUseful() const;

    Type referenced() const {
        auto copy = *this;
        copy.setReference(true);
        return copy;
    }
    
    bool isMutable() const;
    void setMutable(bool b) { mutable_ = b; }

    /// Whether the value will be exactly of the type reprsented by this instance. If this instance represents a class
    /// instance, the instance will be of exactly the type represented by this instance at runtime.
    /// If this instance represents an instance of a final class, this method returns true regardless of whether
    /// inexacted() was called or setExact() was called with false as argument.
    bool isExact() const;
    Type inexacted() const {
        auto copy = *this;
        copy.setExact(false);
        return copy;
    }
    void setExact(bool b) { forceExact_ = b; }

    inline bool operator<(const Type &rhs) const {
        return std::tie(typeContent_, typeDefinition_, rhs.genericArguments_, genericArgumentIndex_,
                        localResolutionConstraint_) < std::tie(rhs.typeContent_, rhs.typeDefinition_,
                                                               rhs.genericArguments_, rhs.genericArgumentIndex_,
                                                               rhs.localResolutionConstraint_);
    }

    inline bool operator==(const Type &rhs) const {
        return std::tie(typeContent_, typeDefinition_, rhs.genericArguments_, genericArgumentIndex_,
                        localResolutionConstraint_) == std::tie(rhs.typeContent_, rhs.typeDefinition_,
                                                                rhs.genericArguments_, rhs.genericArgumentIndex_,
                                                                rhs.localResolutionConstraint_);
    }

    inline bool operator!=(const Type &rhs) const { return !(*this == rhs); }

    /// Returns true iff a value of the given type requires memory management.
    bool isManaged() const;

protected:
    Type(bool isReference, bool isMutable)
        : typeContent_(TypeType::StorageExpectation), isReference_(isReference), mutable_(isMutable) {}
private:
    explicit Type(TypeType t) : typeContent_(t) {}

    struct MakeOptionalType {};
    Type(MakeOptionalType makeOptional, Type type)
        : typeContent_(TypeType::Optional), genericArguments_({ std::move(type) }) {
        assert(genericArguments_.front().type() != TypeType::Optional &&
               genericArguments_.front().type() != TypeType::Box);
    }

    struct MakeErrorType {};
    Type(MakeErrorType makeError, Type enumType, Type value)
        : typeContent_(TypeType::Error), genericArguments_({ std::move(enumType), std::move(value) }) {
            assert(genericArguments_[1].type() != TypeType::Box);
        }

    struct MakeBoxType {};
    Type(MakeBoxType, Type type, Type forType)
        : typeContent_(TypeType::Box), genericArguments_({ std::move(type), std::move(forType) }) {
//            assert(genericArguments_[1].type() == TypeType::Protocol ||
//                   genericArguments_[1].type() == TypeType::Something);
            assert(genericArguments_.front().type() != TypeType::Box);
            assert(genericArguments_[1].type() != TypeType::Box);
        }

    TypeType typeContent_;
    size_t genericArgumentIndex_ = 0;
    TypeDefinition *typeDefinition_ = nullptr;
    Function *localResolutionConstraint_ = nullptr;
    std::vector<Type> genericArguments_;

    TypeDefinition* resolutionConstraint() const;
    Function* localResolutionConstraint() const;

    bool isReference_ = false;
    bool mutable_ = false;
    bool forceExact_ = false;

    void typeName(Type type, const TypeContext &typeContext, std::string &string, Package *package) const;
    bool identicalGenericArguments(Type to, const TypeContext &typeContext, std::vector<CommonTypeFinder> *ctargs) const;
    void sortMultiProtocolType();

    bool isCompatibleToMultiProtocol(const Type &to, const TypeContext &ct, std::vector<CommonTypeFinder> *ctargs) const;
    bool isCompatibleToProtocol(const Type &to, const TypeContext &ct, std::vector<CommonTypeFinder> *ctargs) const;
    bool isCompatibleToCallable(const Type &to, const TypeContext &ct, std::vector<CommonTypeFinder> *ctargs) const;
    bool isCompatibleToTypeAsValue(const Type &to, const TypeContext &tc, std::vector<CommonTypeFinder> *ctargs) const;
};

}  // namespace EmojicodeCompiler

#endif /* Type_hpp */
