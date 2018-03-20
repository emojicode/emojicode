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
class Extension;

enum class TypeType {
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
    Extension,
};

struct MakeOptionalType {};
constexpr MakeOptionalType MakeOptional {};

struct MakeTypeAsValueType {};
constexpr MakeTypeAsValueType MakeTypeAsValue {};

/// Represents the type of variable, an argument or the return value of a Function such as a method, an initializer,
/// or type method.
class Type {
    friend AbstractParser;
    friend Initializer;
    friend Function;
public:
    Type(MakeOptionalType makeOptional, Type type)
            : typeContent_(TypeType::Optional), genericArguments_({ std::move(type) }) {}
    Type(MakeTypeAsValueType makeTypeAsValue, Type type)
            : typeContent_(TypeType::TypeAsValue), genericArguments_({ std::move(type) }) {}

    explicit Type(Class *klass);
    explicit Type(Protocol *protocol);
    explicit Type(Enum *enumeration);
    explicit Type(ValueType *valueType);
    explicit Type(Extension *extension);

    /// Creates a generic variable to the generic argument @c r.
    Type(size_t r, TypeDefinition *resolutionConstraint, bool forceBox)
            : typeContent_(TypeType::GenericVariable), genericArgumentIndex_(r),
              typeDefinition_(resolutionConstraint), forceBox_(forceBox) {}

    /// Creates a local generic variable (generic function) to the generic argument @c r.
    Type(size_t r, Function *function, bool forceBox)
    : typeContent_(TypeType::LocalGenericVariable), genericArgumentIndex_(r),
      localResolutionConstraint_(function), forceBox_(forceBox) {}

    explicit Type(std::vector<Type> protocols)
        : typeContent_(TypeType::MultiProtocol), genericArguments_(std::move(protocols)) {
            sortMultiProtocolType();
        }

    static Type something() { return Type(TypeType::Something); }
    static Type noReturn() { return Type(TypeType::NoReturn); }
    static Type error() { return Type(TypeType::Error); }
    static Type someobject() { return Type(TypeType::Someobject); }
    static Type callableIncomplete() { return Type(TypeType::Callable); }
    
    /// @returns The type of this type, i.e. Protocol, Class instance etc.
    TypeType type() const { return typeContent_; }

    Class* eclass() const;
    Protocol* protocol() const;
    Enum* eenum() const;
    ValueType* valueType() const;
    TypeDefinition* typeDefinition() const;

    /// Returns the size of Emojicode Words this type instance will take in a scope or another type instance.
    int size() const { return 0; }
    /// Returns the storage type that will be used, i.e. the boxing applied in memory.
    StorageType storageType() const;
    /// Unboxes this type.
    /// @throws std::logic_error if unboxing is not possible according to @c requiresBox()
    void unbox() { forceBox_ = false; if (requiresBox()) { throw std::logic_error("Cannot unbox!"); } }
    Type unboxed() const {
        auto copy = *this;
        copy.unbox();
        return copy;
    }
    void forceBox() { forceBox_ = true; }

    bool remotelyStored() const { return false; }

    /// @returns The type this optional contains. If this type is force boxed, so will be the returned type.
    Type optionalType() const {
        auto t = genericArguments_[0];
        if (forceBox_) {
            t.forceBox_ = true;
        }
        return t;
    }
    /// @returns The type itself if type() does not return TypeType::Optional or optionalType() if it does.
    Type unoptionalized() const { return type() == TypeType::Optional ? optionalType() : *this; }

    const Type& typeOfTypeValue() const { return genericArguments_[0]; }

    /// Returns true if this type is compatible to the given other type.
    bool compatibleTo(const Type &to, const TypeContext &tc, std::vector<CommonTypeFinder> *ctargs = nullptr) const;
    /// Whether this instance of Type is considered indentical to the other instance.
    /// Mainly used to determine compatibility of generics.
    bool identicalTo(Type to, const TypeContext &tc, std::vector<CommonTypeFinder> *ctargs) const;

    /// Returns the generic variable index if the type is a Type::GenericVariable or TypeType::LocalGenericVariable.
    /// @throws std::domain_error if the Type is not a generic variable.
    size_t genericVariableIndex() const;

    /// Returns the generic arguments with which this type was specialized.
    const std::vector<Type>& genericArguments() const { return genericArguments_; }
    /// Allows to change a specific generic argument. @c index must be smaller than @c genericArguments().size()
    void setGenericArgument(size_t index, Type value) { genericArguments_[index] = std::move(value); }
    /// True if this type could have generic arguments.
    bool canHaveGenericArguments() const;

    bool canHaveProtocol() const { return type() == TypeType::ValueType || type() == TypeType::Class
        || type() == TypeType::Enum; }

    /// Returns the protocol types of a MultiProtocol
    const std::vector<Type>& protocols() const { return genericArguments_; }

    /// Tries to resolve this type to a non-generic-variable type by using the generic arguments provided in the
    /// TypeContext. This method also tries to resolve generic arguments to non-generic-variable types recursively.
    /// This method can resolve @c Self, @c References and @c LocalReferences. @c GenericVariable will only be resolved
    /// if the TypeContext’s @c calleeType implementation of @c canBeUsedToResolve returns true for the resolution
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
    /// @param typeContext The type context to be used when resolving generic argument names. Can be Nothingeness if the
    /// type is not in a context.
    std::string toString(const TypeContext &typeContext, bool package = true) const;

    /// @returns true if the type always requires a box to store important dynamic type information.
    /// A protocol box, for instance, requires a box to store dynamic type information, while
    /// class instances may, of course, be unboxed.
    /// @note This method determines its return regardless of whether this instance represents a boxed value already.
    bool requiresBox() const;

    /// Returns true if this represents a reference to (a) value(s) of the type represented by this instance.
    /// Values to which references point are normally located on the stack.
    bool isReference() const { return isReference_; }
    void setReference(bool v = true) { isReference_ = v; }
    /// Returns true if it makes sense to pass this value with the given storage type per reference to avoid copying.
    bool isReferencable() const;

    bool isMutable() const { return mutable_; }
    void setMutable(bool b) { mutable_ = b; }

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
protected:
    Type(bool isReference, bool forceBox, bool isMutable)
        : typeContent_(TypeType::StorageExpectation), isReference_(isReference),
          mutable_(isMutable), forceBox_(forceBox) {}
private:
    explicit Type(TypeType t) : typeContent_(t) {}

    TypeType typeContent_;
    size_t genericArgumentIndex_ = 0;
    TypeDefinition *typeDefinition_ = nullptr;
    Function *localResolutionConstraint_ = nullptr;
    std::vector<Type> genericArguments_;

    bool isReference_ = false;
    bool mutable_ = true;
    /// Indicates that the value is boxed although the type would normally not require boxing. Used with generics
    bool forceBox_ = false;

    void typeName(Type type, const TypeContext &typeContext, std::string &string, bool package) const;
    bool identicalGenericArguments(Type to, const TypeContext &typeContext, std::vector<CommonTypeFinder> *ctargs) const;
    Type resolveReferenceToBaseReferenceOnSuperArguments(const TypeContext &typeContext) const;
    void sortMultiProtocolType();

    bool isCompatibleToMultiProtocol(const Type &to, const TypeContext &ct, std::vector<CommonTypeFinder> *ctargs) const;
    bool isCompatibleToProtocol(const Type &to, const TypeContext &ct, std::vector<CommonTypeFinder> *ctargs) const;
    bool isCompatibleToCallable(const Type &to, const TypeContext &ct, std::vector<CommonTypeFinder> *ctargs) const;
};

}  // namespace EmojicodeCompiler

#endif /* Type_hpp */
