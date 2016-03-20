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

enum TypeType {
    /** The type is of the class provided. */
    TT_CLASS = 0,
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
    TT_CALLABLE
};

enum TypeDynamism {
    NoDynamism = 0,
    AllowGenericTypeVariables = 0b1,
    AllowDynamicClassType = 0b10
};

struct TypeContext;
class Procedure;

struct Type {
public:
    /**
     * Tries to fetch a type by its name and namespace.
     * @return The found type. If no type is found the return value is undefined and the value pointed to by @c existent will be set to false, otherwise to true.
     */
    static Type fetchRawType(EmojicodeChar name, EmojicodeChar enamespace, bool optional, const Token *token, bool *existent);
    
    /** Reads a type name and stores it into the given pointers. */
    static const Token* parseTypeName(EmojicodeChar *typeName, EmojicodeChar *ns, bool *optional, EmojicodeChar currentNamespace);
    
    /** Reads a type name and stores it into the given pointers. */
    static Type parseAndFetchType(TypeContext tc, EmojicodeChar theNamespace, TypeDynamism dynamism, bool *dynamicType = nullptr);
    
    Type(TypeType t, bool o) : optional(o), type(t) {}
    Type(TypeType t, bool o, uint16_t r) : optional(o), type(t), reference(r) {}
    Type(Class *c, bool o);
    Type(Class *c) : Type(c, false) {};
    Type(Protocol *p, bool o) : optional(o), type(TT_PROTOCOL), protocol(p) {}
    Type(Enum *e, bool o) : optional(o), type(TT_ENUM), eenum(e) {}
    
    bool optional;
    TypeType type;
    union {
        Class *eclass;
        Protocol *protocol;
        Enum *eenum;
        uint16_t reference;
        uint32_t arguments;
    };
    std::vector<Type> genericArguments;
    
    bool compatibleTo(Type to, TypeContext tc);
    
    /** Returns the name of the package to which this type belongs. */
    const char* typePackage();
    
    /** Whether the given type is a valid argument for the generic argument at index @c i. */
    void validateGenericArgument(Type type, uint16_t i, TypeContext tc, const Token *token);

    /** Called by @c parseAndFetchType and in the class parser. You usually should not call this method. */
    void parseGenericArguments(TypeContext tc, EmojicodeChar theNamespace, TypeDynamism dynamism, const Token *errorToken);
    
    /**
     * Returns a depp string representation of the given type.
     * @param contextType The contextType. Can be Nothingeness if the type is not in a context.
     * @param includeNsAndOptional Whether to include optional indicators and the namespaces.
     */
    std::string toString(TypeContext contextType, bool includeNsAndOptional) const;
    
    /** Returns this type as a non-reference type by resolving it on the given type @c o if necessary. */
    Type resolveOn(TypeContext contextType);
    
    Type typeConstraintForReference(TypeContext ct);
private:
    void typeName(Type type, TypeContext typeContext, bool includeNsAndOptional, std::string *string) const;
    Type resolveOnSuperArguments(Class *c, bool *resolved);
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
