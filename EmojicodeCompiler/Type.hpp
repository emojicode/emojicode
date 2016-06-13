//
//  Type.h
//  Emojicode
//
//  Created by Theo Weidmann on 29/12/15.
//  Copyright © 2015 Theo Weidmann. All rights reserved.
//

#ifndef Type_hpp
#define Type_hpp

/** The Emoji representing the standard ("global") enamespace. */
#define globalNamespace E_LARGE_RED_CIRCLE

#include <vector>
#include <string>

class TypeDefinition;
class Enum;
class Class;
class Protocol;
class Package;
class TypeDefinitionFunctional;
class TypeContext;
class Function;
class ValueType;
struct CommonTypeFinder;

enum TypeType {
    TT_CLASS,
    TT_PROTOCOL,
    TT_ENUM,
    TT_VALUE_TYPE,
    TT_NOTHINGNESS,
    /** Maybe everything. */
    TT_SOMETHING,
    /** Any Object */
    TT_SOMEOBJECT,
    /** Used with generics */
    TT_REFERENCE,
    TT_LOCAL_REFERENCE,
    TT_CALLABLE,
    
    TT_SELF
};

enum TypeDynamism {
    /** No dynamism is allowed or no dynamism was used. */
    NoDynamism = 0,
    /** No kind of dynamism is allowed. This value never comes from a call to @c parseAndFetchType . */
    AllKindsOfDynamism = 0b11,
    /** Generic Variables are allowed or were used. */
    GenericTypeVariables = 0b1,
    /** Self is allowed or was used. */
    Self = 0b10
};

class Type {
public:
    Type(TypeType t, bool o) : type_(t), optional_(o) {}
    Type(TypeType t, bool o, uint16_t r, TypeDefinitionFunctional *c)
        : reference(r), resolutionConstraint(c), type_(t), optional_(o) {}
    explicit Type(Class *c) : Type(c, false) {};
    Type(Class *c, bool o);
    Type(Protocol *p, bool o);
    Type(Enum *e, bool o);
    Type(ValueType *v, bool o);
    
    /** Returns the type of this type. Whether it’s an integer, class, etc. */
    TypeType type() const { return type_; }
    /** Whether this type of type could have generic arguments. */
    bool canHaveGenericArguments() const;
    /** Returns the represented TypeDefinitonWithGenerics by using a cast. */
    TypeDefinitionFunctional* typeDefinitionFunctional() const;
    Class* eclass() const;
    Protocol* protocol() const;
    Enum* eenum() const;
    ValueType* valueType() const;
    TypeDefinition* typeDefinition() const;
    
    union {
        struct {
            uint16_t reference;
            TypeDefinitionFunctional *resolutionConstraint;
        };
        uint32_t arguments;
    };

    std::vector<Type> genericArguments;
    
    /** Whether the type is an optional. */
    bool optional() const { return optional_; }
    /** Marks this type as optional. You can never make an optional type un-optional. */
    void setOptional() { optional_ = true; }
    /** Returns a copy of this type but with @c optional set to @c false. */
    Type copyWithoutOptional() const;
    
    /** If this type is compatible to the given other type. */
    bool compatibleTo(Type to, TypeContext tc, std::vector<CommonTypeFinder> *ctargs = nullptr) const;
    /** 
     * Whether this type is considered indentical to the other type. 
     * Mainly used to determine compatibility of generics.
     */
    bool identicalTo(Type to, TypeContext tc, std::vector<CommonTypeFinder> *ctargs) const;
    
    /** Returns this type as a non-reference type by resolving it on the given type @c o if necessary. */
    Type resolveOn(TypeContext contextType, bool resolveSelf = true) const;
    Type resolveOnSuperArgumentsAndConstraints(TypeContext ct, bool resolveSelf = true) const;
    
    /** Returns the name of the package to which this type belongs. */
    const char* typePackage();
    /**
     * Returns a depp string representation of the given type.
     * @param contextType The contextType. Can be Nothingeness if the type is not in a context.
     * @param includeNsAndOptional Whether to include optional indicators and the namespaces.
     */
    std::string toString(TypeContext contextType, bool includeNsAndOptional) const;
private:
    TypeDefinition *typeDefinition_;
    TypeType type_;
    bool optional_;
    void typeName(Type type, TypeContext typeContext, bool includePackageAndOptional, std::string &string) const;
    bool identicalGenericArguments(Type to, TypeContext ct, std::vector<CommonTypeFinder> *ctargs) const;
};

#define typeInteger (Type(VT_INTEGER, false))
#define typeBoolean (Type(VT_BOOLEAN, false))
#define typeSymbol (Type(VT_SYMBOL, false))
#define typeFloat (Type(VT_DOUBLE, false))
#define typeSomething (Type(TT_SOMETHING, false))
#define typeNothingness (Type(TT_NOTHINGNESS, false))
#define typeSomeobject (Type(TT_SOMEOBJECT, false))

#endif /* Type_hpp */
