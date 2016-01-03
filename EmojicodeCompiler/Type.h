//
//  Type.h
//  Emojicode
//
//  Created by Theo Weidmann on 29/12/15.
//  Copyright © 2015 Theo Weidmann. All rights reserved.
//

#ifndef Type_h
#define Type_h

/** The Emoji representing the standard ("global") namespace. */
#define globalNamespace E_LARGE_RED_CIRCLE

typedef enum {
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
} TypeType;

typedef struct Type {
    bool optional;
    TypeType type;
    union {
        Class *class;
        Protocol *protocol;
        Enum *eenum;
        uint16_t reference;
        uint32_t arguments;
    };
    struct Type *genericArguments;
} Type;

typedef enum {
    NoDynamism = 0,
    AllowGenericTypeVariables = 0b1,
    AllowDynamicClassType = 0b10
} TypeDynamism;

#define typeInteger ((Type){false, TT_INTEGER})
#define typeBoolean ((Type){false, TT_BOOLEAN})
#define typeSymbol ((Type){false, TT_SYMBOL})
#define typeSomething ((Type){false, TT_SOMETHING})
#define typeLong ((Type){false, TT_LONG})
#define typeFloat ((Type){false, TT_DOUBLE})
#define typeNothingness ((Type){false, TT_NOTHINGNESS})
#define typeSomeobject ((Type){false, TT_SOMEOBJECT})

/** Wrapps a class into a Type */
extern Type typeForClass(Class *class);

/** Wrapps a class into an optional Type */
extern Type typeForClassOptional(Class *class);

/** Determines if the type is wrapping @c CL_NOTHINGNESS */
extern bool typeIsNothingness(Type a);

extern Type typeForProtocol(Protocol *protocol);

extern Type typeForProtocolOptional(Protocol *protocol);

extern Type resolveTypeReferences(Type t, Type o);

extern const char* typePackage(Type type);

/**
 * Whether two types are compatible.
 * A return of true means that a can be casted to @c to.
 */
bool typesCompatible(Type a, Type to, Type parentType);

extern int initializeAndCopySuperGenericArguments(Type *type);
extern void checkEnoughGenericArguments(uint16_t count, Type type, Token *token);
extern void validateGenericArgument(Type ta, uint16_t i, Type type, Token *token);

/**
 * Fetches a type by its name and namespace or throws an error.
 * @param token The token is used when throwing an error.
 */
extern Type fetchRawType(EmojicodeChar name, EmojicodeChar namespace, bool optional, Token *token, bool *existent);

/** Reads a type name and stores it into the given pointers. */
extern Token* parseTypeName(EmojicodeChar *typeName, EmojicodeChar *ns, bool *optional, EmojicodeChar currentNamespace);

/** Reads a type name and stores it into the given pointers. */
extern Type parseAndFetchType(Class *cl, EmojicodeChar theNamespace, TypeDynamism dynamism, bool *dynamicType);

extern void emitCommonTypeWarning(Type *commonType, bool *firstTypeFound, Token *token);

extern void determineCommonType(Type t, Type *commonType, bool *firstTypeFound, Type parentType);

#endif /* Type_h */
