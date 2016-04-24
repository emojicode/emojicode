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

class Package;

enum TypeType {
    TT_CLASS,
    TT_PROTOCOL,
    TT_ENUM,
    
    TT_BOOLEAN,
    TT_INTEGER,
    TT_SYMBOL,
    TT_DOUBLE,
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

struct TypeContext;
class Procedure;

class Type {
public:
    /** Reads a type name and stores it into the given pointers. */
    static const Token* parseTypeName(EmojicodeChar *typeName, EmojicodeChar *ns, bool *optional);
    /** Reads a type name and stores it into the given pointers. */
    static Type parseAndFetchType(TypeContext tc, TypeDynamism dynamism, Package *package,
                                  TypeDynamism *dynamicType = nullptr, bool allowProtocolsUsingSelf = false);
    
    Type(TypeType t, bool o) : optional_(o), type_(t) {}
    Type(TypeType t, bool o, uint16_t r, TypeDefinitionWithGenerics *c)
        : optional_(o), type_(t), reference(r), resolutionConstraint(c) {}
    Type(Class *c, bool o);
    Type(Class *c) : Type(c, false) {};
    Type(Protocol *p, bool o) : optional_(o), type_(TT_PROTOCOL), protocol(p) {}
    Type(Enum *e, bool o) : optional_(o), type_(TT_ENUM), eenum(e) {}
    
    
    /** Returns the type of this type. Whether it’s an integer, class, etc. */
    TypeType type() const { return type_; }
    /** Whether this type of type could have generic arguments. */
    bool canHaveGenericArguments() const;
    /** Returns the represented TypeDefinitonWithGenerics by using a cast. */
    TypeDefinitionWithGenerics* typeDefinitionWithGenerics() const;
    
    union {
        Class *eclass;
        Protocol *protocol;
        Enum *eenum;
        struct {
            uint16_t reference;
            TypeDefinitionWithGenerics *resolutionConstraint;
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
    bool compatibleTo(Type to, TypeContext tc) const;
    /** 
     * Whether this type is considered indentical to the other type. 
     * Mainly used to determine compatibility of generics.
     */
    bool identicalTo(Type to) const;
    
    /** Called by @c parseAndFetchType and in the class parser. You usually should not call this method. */
    void parseGenericArguments(TypeContext tc, TypeDynamism dynamism, Package *package, const Token *errorToken);
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
    TypeType type_;
    bool optional_;
    void typeName(Type type, TypeContext typeContext, bool includePackageAndOptional, std::string &string) const;
};

struct TypeContext {
public:
    TypeContext(Type nt) : normalType(nt) {};
    TypeContext(Type nt, Procedure *p) : normalType(nt), p(p) {};
    TypeContext(Type nt, Procedure *p, std::vector<Type> *args) : normalType(nt), p(p), procedureGenericArguments(args) {};
    
    Type normalType;
    Procedure *p = nullptr;
    std::vector<Type> *procedureGenericArguments = nullptr;
};

#define typeInteger (Type(TT_INTEGER, false))
#define typeBoolean (Type(TT_BOOLEAN, false))
#define typeSymbol (Type(TT_SYMBOL, false))
#define typeSomething (Type(TT_SOMETHING, false))
#define typeLong (Type(TT_LONG, false))
#define typeFloat (Type(TT_DOUBLE, false))
#define typeNothingness (Type(TT_NOTHINGNESS, false))
#define typeSomeobject (Type(TT_SOMEOBJECT, false))

struct CommonTypeFinder {
    /** Tells the common type finder about the type of another element in the collection/data structure. */
    void addType(Type t, TypeContext typeContext);
    /** Returns the common type and issues a warning at @c warningToken if the common type is ambigious. */
    Type getCommonType(const Token *warningToken);
private:
    bool firstTypeFound = false;
    Type commonType = typeSomething;
};

#endif /* Type_hpp */
