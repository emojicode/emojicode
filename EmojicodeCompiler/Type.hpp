//
//  Type.h
//  Emojicode
//
//  Created by Theo Weidmann on 29/12/15.
//  Copyright © 2015 Theo Weidmann. All rights reserved.
//

#ifndef Type_hpp
#define Type_hpp

#include <vector>
#include <string>
#include <stdexcept>
#include "EmojicodeCompiler.hpp"
#include "StorageType.hpp"

/** The Emoji representing the standard ("global") namespace. */
const EmojicodeString globalNamespace = EmojicodeString(E_LARGE_RED_CIRCLE);

class TypeDefinition;
class Enum;
class Class;
class Protocol;
class Package;
class TypeDefinitionFunctional;
class TypeContext;
class ValueType;
class Function;
class CommonTypeFinder;
class AbstractParser;
class Initializer;

enum class TypeContent {
    Class,
    MultiProtocol,
    Protocol,
    Enum,
    ValueType,
    Nothingness,
    /** Maybe everything. */
    Something,
    /** Any Object */
    Someobject,
    /** Used with generics */
    Reference,
    LocalReference,
    Callable,
    Self,
    Error,
    StorageExpectation,
};

struct ObjectVariableInformation {
    ObjectVariableInformation(int index, ObjectVariableType type) : index(index), type(type) {}
    ObjectVariableInformation(int index, int condition, ObjectVariableType type)
        : index(index), conditionIndex(condition), type(type) {}
    int index;
    int conditionIndex;
    ObjectVariableType type;
};

enum class TypeDynamism {
    /** No dynamism is allowed or no dynamism was used. */
    None = 0,
    /** No kind of dynamism is allowed. This value never comes from a call to @c parseAndFetchType . */
    AllKinds = 0b11,
    /** Generic Variables are allowed or were used. */
    GenericTypeVariables = 0b1,
    /** Self is allowed or was used. */
    Self = 0b10
};

inline TypeDynamism operator&(TypeDynamism a, TypeDynamism b) {
    return static_cast<TypeDynamism>(static_cast<int>(a) & static_cast<int>(b));
}

inline TypeDynamism operator|(TypeDynamism a, TypeDynamism b) {
    return static_cast<TypeDynamism>(static_cast<int>(a) | static_cast<int>(b));
}

/// Represents a Type Instance or an expectations of a Type Instance
class Type {
    friend TypeDefinitionFunctional;
    friend AbstractParser;
    friend Initializer;
    friend Function;
public:
    Type(Class *klass, bool optional);
    Type(Protocol *protocol, bool optional);
    Type(Enum *enumeration, bool optional);
    Type(ValueType *valueType, bool optional, bool isMutable);
    /// Creates a reference to the generic argument @c r which can only be resolved on types which declare themselves
    /// to be suitable. (See @c canBeUsedToResolve).
    Type(TypeContent t, bool optional, int r, TypeDefinitionFunctional *resolutionConstraint)
        : reference_(r), resolutionConstraint_(resolutionConstraint), typeContent_(t), optional_(optional) {}
    /// Creates a reference to the generic argument @c r which can be resolved to an argument given to @c function.
    Type(TypeContent t, bool optional, int r, Function *function)
        : reference_(r), localResolutionConstraint_(function), typeContent_(t), optional_(optional) {}
    Type(std::vector<Type> protocols, bool optional)
        : typeContent_(TypeContent::MultiProtocol), genericArguments_(protocols), optional_(optional) {
            sortMultiProtocolType();
        }
    
    static Type integer() { return Type(VT_INTEGER, false, false); }
    static Type boolean() { return Type(VT_BOOLEAN, false, false); }
    static Type symbol() { return Type(VT_SYMBOL, false, false); }
    static Type doubl() { return Type(VT_DOUBLE, false, false); }
    static Type something() { return Type(TypeContent::Something, false); }
    static Type nothingness() { return Type(TypeContent::Nothingness, false); }
    static Type error() { return Type(TypeContent::Error, false); }
    static Type someobject(bool optional = false) { return Type(TypeContent::Someobject, optional); }
    static Type callableIncomplete(bool optional = false) { return Type(TypeContent::Callable, optional); }
    static Type self(bool optional = false) { return Type(TypeContent::Self, optional); }
    
    /** Returns the type of this type. Whether it’s an integer, class, etc. */
    TypeContent type() const { return typeContent_; }

    /** Returns the represented TypeDefinitionWithGenerics by using a cast. */
    TypeDefinitionFunctional* typeDefinitionFunctional() const;
    Class* eclass() const;
    Protocol* protocol() const;
    Enum* eenum() const;
    ValueType* valueType() const;
    TypeDefinition* typeDefinition() const;
    
    /// Returns the size of Emojicode Words this type instance will take in a scope or another type instance.
    int size() const;
    /// Returns the storage type that will be used, i.e. the boxing applied in memory.
    StorageType storageType() const;
    /// Returns a numeric identifier used to differentiate Nothingness, Object References and Value Types at run-time.
    EmojicodeInstruction boxIdentifier() const;
    /// Unboxes this type.
    /// @throws std::logic_error if unboxing is not possible according to @c requiresBox()
    void unbox() { forceBox_ = false; if (requiresBox()) throw std::logic_error("Cannot unbox!"); }
    void forceBox() { forceBox_ = true; }

    /// True if the type is an optional.
    bool optional() const { return optional_; }
    /// Marks this type as optional.
    void setOptional() { optional_ = true; }
    /// Returns a copy of this type but with @c optional set to @c false.
    Type copyWithoutOptional() const;

    /// Returns true if this type is compatible to the given other type.
    bool compatibleTo(Type to, const TypeContext &tc, std::vector<CommonTypeFinder> *ctargs = nullptr) const;
    /// Whether this instance of Type is considered indentical to the other instance.
    /// Mainly used to determine compatibility of generics.
    bool identicalTo(Type to, const TypeContext &tc, std::vector<CommonTypeFinder> *ctargs) const;

    /// Returns the reference if the type is a @c Reference.
    /// @throws std::domain_error if the Type is not a reference.
    int reference() const;

    /// Appends all records necessary to inform the garbage collector about any object variables inside this type at
    /// index @c index to the end of @c information by constructing an instance of @c T with a constructors as
    /// those provided by @c ObjectVariableInformation. @c args are passed forward to the constructors at the end of the
    /// argument list.
    template <typename T, typename... Us>
    void objectVariableRecords(int index, std::vector<T> &information, Us... args) const;

    /// Returns the generic arguments with which this type was specialized.
    const std::vector<Type>& genericArguments() const { return genericArguments_; }
    /// Allows to change a specific generic argument. @c index must be smaller than @c genericArguments().size()
    void setGenericArgument(size_t index, Type value) { genericArguments_[index] = value; }
    /// True if this type could have generic arguments.
    bool canHaveGenericArguments() const;

    bool canHaveProtocol() const { return type() == TypeContent::ValueType || type() == TypeContent::Class
        || type() == TypeContent::Enum; }

    /// Returns the protocol types of a MultiProtocol
    const std::vector<Type>& protocols() const { return genericArguments_; }

    /// Forbids the usage of this instance of Type to resolve @c Self.
    Type& disableSelfResolving() { resolveSelfOn_ = false; return *this; }
    /// Tries to resolve this type to a non-refrence type by using the generic arguments provided in the TypeContext.
    /// This method also tries to resolve generic arguments to non-reference types recursively.
    /// This method can resolve @c Self, @c References and @c LocalReferences. @c References will only be resolved if
    /// the TypeContext’s @c calleeType implementation of @c canBeUsedToResolve returns true for the resolution
    /// constraint, thus only if the reference is inteded to be resolved on the given type. @c Self will only be
    /// resolved if @c disableSelfResolving() wasn’t called on the calleeType before.
    Type resolveOn(const TypeContext &typeContext, bool resolveSelf = true) const;
    /**
     * Used to get as mutch information about a reference type as possible without using the generic arguments of
     * the type context’s callee.
     * This method is intended to be used to determine type compatibility while e.g. compiling generic classes.
     */
    Type resolveOnSuperArgumentsAndConstraints(const TypeContext &typeContext, bool resolveSelf = true) const;
    
    /// Returns the name of the package to which this type belongs.
    std::string typePackage();
    /// Returns a string representation of this type.
    /// @param typeContext The type context to be used when resolving generic argument names. Can be Nothingeness if the
    /// type is not in a context.
    /// @param optionalAndPackages Whether to include optional indicators and package names.
    std::string toString(const TypeContext &typeContext, bool optionalAndPackages) const;
    
    void setMeta(bool meta) { meta_ = meta; }
    bool meta() const { return meta_; }

    /// Returns true if this represents a reference to (a) value(s) of the type represented by this instance.
    /// Values to which references point are normally located on the stack.
    bool isReference() const { return isReference_; }
    void setReference(bool v = true) { isReference_ = v; }
    /// Returns true if it makes sense to pass this value with the given storage type per reference to avoid copying.
    bool isReferenceWorthy() const;
    /// Returns true if the type requires a box to store important dynamic type information.
    /// A protocol box, for instance, requires a box to store dynamic type information, while
    /// class instances may, of course, be unboxed.
    /// @note This method determines its return regardless of whether this instance represents a boxed value.
    bool requiresBox() const;

    bool isMutable() const { return mutable_; }
    void setMutable(bool b) { mutable_ = b; }

    bool allowsMetaType();
protected:
    Type(bool isReference, bool forceBox, bool isMutable)
        : typeContent_(TypeContent::StorageExpectation), optional_(false), isReference_(isReference),
          mutable_(isMutable), forceBox_(forceBox) {}
private:
    Type(TypeContent t, bool o) : typeContent_(t), optional_(o) {}
    int reference_;
    union {
        TypeDefinition *typeDefinition_;
        TypeDefinitionFunctional *resolutionConstraint_;
        Function *localResolutionConstraint_;
    };
    TypeContent typeContent_;
    std::vector<Type> genericArguments_;
    bool optional_;
    bool resolveSelfOn_ = true;
    bool meta_ = false;
    bool isReference_ = false;
    bool mutable_ = true;
    /// Indicates that the value is boxed although the type would normally not require boxing. Used with generics
    bool forceBox_ = false;

    void typeName(Type type, const TypeContext &typeContext, bool includePackageAndOptional, std::string &string) const;
    bool identicalGenericArguments(Type to, const TypeContext &typeContext, std::vector<CommonTypeFinder> *ctargs) const;
    Type resolveReferenceToBaseReferenceOnSuperArguments(const TypeContext &typeContext) const;
    void sortMultiProtocolType();
};

#endif /* Type_hpp */
