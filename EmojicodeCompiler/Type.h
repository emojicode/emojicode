//
//  Type.h
//  Emojicode
//
//  Created by Theo Weidmann on 29/12/15.
//  Copyright © 2015 Theo Weidmann. All rights reserved.
//

#ifndef Type_h
#define Type_h

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
    TT_CALLABLE
};

enum TypeDynamism {
    NoDynamism = 0,
    AllowGenericTypeVariables = 0b1,
    AllowDynamicClassType = 0b10
};

struct Type {
public:
    Type(TypeType t, bool o) : optional(o), type(t) {}
    Type(TypeType t, bool o, uint16_t r) : optional(o), type(t), reference(r) {}
    Type(Class *c, bool o);
    Type(Class *c) : Type(c, false) {};
    Type(Protocol *p, bool o) : optional(o), protocol(p), type(TT_PROTOCOL) {}
    Type(Enum *e, bool o) : optional(o), eenum(e), type(TT_ENUM) {}
    
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
    
    bool compatibleTo(Type to, Type contenxtType);
    
    /** Returns the name of the package to which this type belongs. */
    const char* typePackage();
    
    /** Whether the given type is a valid argument for the generic argument at index @c i. */
    void validateGenericArgument(Type type, uint16_t i, Token *token);

    /** Called by @c parseAndFetchType and in the class parser. You usually should not call this method. */
    void parseGenericArguments(Class *eclass, EmojicodeChar theNamespace, TypeDynamism dynamism, Token *errorToken);
    
    /**
     * Returns a depp string representation of the given type.
     * @param includeNsAndOptional Whether to include optional indicators and the namespaces.
     */
    std::string toString(Type contextType, bool includeNsAndOptional) const;
private:
    void typeName(Type type, Type parentType, bool includeNsAndOptional, std::string *string) const;
    Type typeConstraintForReference(Class *c);
    Type resolveOnSuperArguments(Class *c, bool *resolved);
};

#define typeInteger (Type(TT_INTEGER, false))
#define typeBoolean (Type(TT_BOOLEAN, false))
#define typeSymbol (Type(TT_SYMBOL, false))
#define typeSomething (Type(TT_SOMETHING, false))
#define typeLong (Type(TT_LONG, false))
#define typeFloat (Type(TT_DOUBLE, false))
#define typeNothingness (Type(TT_NOTHINGNESS, false))
#define typeSomeobject (Type(TT_SOMEOBJECT, false))

extern Type resolveTypeReferences(Type t, Type o);

/**
 * Fetches a type by its name and enamespace or throws an error.
 * @param token The token is used when throwing an error.
 */
extern Type fetchRawType(EmojicodeChar name, EmojicodeChar enamespace, bool optional, Token *token, bool *existent);

/** Reads a type name and stores it into the given pointers. */
extern Token* parseTypeName(EmojicodeChar *typeName, EmojicodeChar *ns, bool *optional, EmojicodeChar currentNamespace);

/** Reads a type name and stores it into the given pointers. */
extern Type parseAndFetchType(Class *cl, EmojicodeChar theNamespace, TypeDynamism dynamism, bool *dynamicType);

struct CommonTypeFinder {
    /** Tells the common type finder about the type of another element in the collection/data structure. */
    void addType(Type t, Type contextType);
    /** Returns the common type and issues a warning at @c warningToken if the common type is ambigious. */
    Type getCommonType(Token *warningToken);
private:
    bool firstTypeFound = false;
    Type commonType = typeSomething;
};

#endif /* Type_h */
