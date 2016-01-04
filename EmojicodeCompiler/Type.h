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

struct Type {
public:
    Type(TypeType t, bool o) : optional(o), type(t) {}
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
private:
    Type typeConstraintForReference(Class *c);
    Type resolveOnSuperArguments(Class *c, bool *resolved);
};

typedef enum {
    NoDynamism = 0,
    AllowGenericTypeVariables = 0b1,
    AllowDynamicClassType = 0b10
} TypeDynamism;

#define typeInteger (Type(TT_INTEGER, false))
#define typeBoolean (Type(TT_BOOLEAN, false))
#define typeSymbol (Type(TT_SYMBOL, false))
#define typeSomething (Type(TT_SOMETHING, false))
#define typeLong (Type(TT_LONG, false))
#define typeFloat (Type(TT_DOUBLE, false))
#define typeNothingness (Type(TT_NOTHINGNESS, false))
#define typeSomeobject (Type(TT_SOMEOBJECT, false))

/** Wrapps a eclass into a Type */
extern Type typeForClass(Class *eclass);

/** Wrapps a eclass into an optional Type */
extern Type typeForClassOptional(Class *eclass);

/** Determines if the type is wrapping @c CL_NOTHINGNESS */
extern bool typeIsNothingness(Type a);



extern Type resolveTypeReferences(Type t, Type o);

extern int initializeAndCopySuperGenericArguments(Type *type);
extern void checkEnoughGenericArguments(uint16_t count, Type type, Token *token);

/**
 * Fetches a type by its name and enamespace or throws an error.
 * @param token The token is used when throwing an error.
 */
extern Type fetchRawType(EmojicodeChar name, EmojicodeChar enamespace, bool optional, Token *token, bool *existent);

/** Reads a type name and stores it into the given pointers. */
extern Token* parseTypeName(EmojicodeChar *typeName, EmojicodeChar *ns, bool *optional, EmojicodeChar currentNamespace);

/** Reads a type name and stores it into the given pointers. */
extern Type parseAndFetchType(Class *cl, EmojicodeChar theNamespace, TypeDynamism dynamism, bool *dynamicType);

extern void emitCommonTypeWarning(Type *commonType, bool *firstTypeFound, Token *token);

extern void determineCommonType(Type t, Type *commonType, bool *firstTypeFound, Type parentType);

#endif /* Type_h */
