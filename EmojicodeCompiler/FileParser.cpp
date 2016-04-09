//
//  ClassParser.c
//  Emojicode
//
//  Created by Theo Weidmann on 05.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include <libgen.h>
#include <cstring>
#include <climits>
#include <vector>
#include "FileParser.hpp"
#include "Procedure.hpp"
#include "Lexer.hpp"
#include "utf8.h"
#include "Class.hpp"


//MARK: Tips

/**
 * Use this function to determine if the user has choosen a bad method/initializer name. It puts a warning if a reserved name is used.
 * @param place The place in code (like "method")
 */
void reservedEmojis(const Token *token, const char *place) {
    EmojicodeChar name = token->value[0];
    switch (name) {
        case E_CUSTARD:
        case E_DOUGHNUT:
        case E_SHORTCAKE:
        case E_CHOCOLATE_BAR:
        case E_COOKING:
        case E_COOKIE:
        case E_LOLLIPOP:
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS:
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY:
        case E_RED_APPLE:
        case E_BEER_MUG:
        case E_CLINKING_BEER_MUGS:
        case E_LEMON:
        case E_GRAPES:
        case E_STRAWBERRY:
        case E_BLACK_SQUARE_BUTTON:
        case E_LARGE_BLUE_DIAMOND:
        case E_DOG:
        case E_HIGH_VOLTAGE_SIGN:
        case E_CLOUD:
        case E_BANANA:
        case E_HONEY_POT:
        case E_SOFT_ICE_CREAM:
        case E_ICE_CREAM:
        case E_TANGERINE:
        case E_WATERMELON:
        case E_AUBERGINE:
        case E_SPIRAL_SHELL:
        case E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE:
        case E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE_WITH_VERTICAL_BAR: {
            ecCharToCharStack(name, nameString);
            compilerError(token, "%s is reserved and cannot be used as %s name.", nameString, place);
        }
    }
}

//MARK: Utilities

static void checkTypeValidity(EmojicodeChar name, EmojicodeChar enamespace,
                              bool optional, const Token *token, Package *package) {
    if (optional) {
        compilerError(token, "🍬 cannot be declared as type.");
    }
    bool existent;
    auto type = package->fetchRawType(name, enamespace, optional, token, &existent);
    if (existent) {
        auto str = type.toString(typeNothingness, true);
        compilerError(currentToken, "Type %s is already defined.", str.c_str());
    }
}

static bool hasAttribute(EmojicodeChar attributeName, const Token **token) {
    if ((*token)->value[0] == attributeName) {
        *token = consumeToken(IDENTIFIER);
        return true;
    }
    return false;
}

static void invalidAttribute(bool set, EmojicodeChar attributeName, const Token *token) {
    if (set) {
        ecCharToCharStack(attributeName, es)
        compilerError(token, "Inapplicable attribute %s.", es);
    }
}

static AccessLevel readAccessLevel(const Token **token) {
    AccessLevel access;
    switch ((*token)->value[0]) {
        case E_CLOSED_LOCK_WITH_KEY:
            *token = consumeToken(IDENTIFIER);
            access = PROTECTED;
            break;
        case E_LOCK:
            *token = consumeToken(IDENTIFIER);
            access = PRIVATE;
            break;
        case E_OPEN_LOCK:
            *token = consumeToken(IDENTIFIER);
        default:
            access = PUBLIC;
    }
    return access;
}

void parseProtocol(Package *pkg, const Token *documentationToken, bool exported) {
    static uint_fast16_t index = 0;
    
    EmojicodeChar name, enamespace;
    bool optional;
    const Token *classNameToken = Type::parseTypeName(&name, &enamespace, &optional);
    
    checkTypeValidity(name, enamespace, optional, classNameToken, pkg);
    
    if (index == UINT16_MAX) {
        compilerError(classNameToken, "You exceeded the limit of 65,535 protocols.");
    }
    
    auto protocol = new Protocol(name, index++, pkg, documentationToken);
    
    pkg->registerType(Type(protocol, false), name, enamespace);
    if (exported) {
        pkg->exportType(Type(protocol, false), name);
    }
    
    const Token *token = consumeToken(IDENTIFIER);
    if (token->value[0] != E_GRAPES) {
        ecCharToCharStack(token->value[0], s);
        compilerError(token, "Expected 🍇 but found %s instead.", s);
    }
    
    while (token = consumeToken(), !(token->type == IDENTIFIER && token->value[0] == E_WATERMELON)) {
        const Token *documentationToken = nullptr;
        if (token->type == DOCUMENTATION_COMMENT) {
            documentationToken = token;
            token = consumeToken();
        }
        token->forceType(IDENTIFIER);
        
        bool deprecated = hasAttribute(E_WARNING_SIGN, &token);
        
        if (token->value[0] != E_PIG) {
            compilerError(token, "Only method declarations are allowed inside a protocol.");
        }
        
        const Token *methodName = consumeToken(IDENTIFIER);
        
        Type returnType = typeNothingness;
        auto method = new Method(methodName->value[0], PUBLIC, false, nullptr, pkg, methodName,
                                 false, documentationToken, deprecated);
        method->parseArgumentList(typeNothingness, pkg);
        method->parseReturnType(typeNothingness, pkg);
        
        protocol->addMethod(method);
    }
}

void parseEnum(Package *pkg, const Token *documentationToken, bool exported) {
    EmojicodeChar name, enamespace;
    bool optional;
    const Token *enumNameToken = Type::parseTypeName(&name, &enamespace, &optional);
    
    checkTypeValidity(name, enamespace, optional, enumNameToken, pkg);
    
    Enum *eenum = new Enum(name, pkg, documentationToken);

    pkg->registerType(Type(eenum, false), name, enamespace);
    if (exported) {
        pkg->exportType(Type(eenum, false), name);
    }
    
    const Token *token = consumeToken(IDENTIFIER);
    if (token->value[0] != E_GRAPES) {
        ecCharToCharStack(token->value[0], s);
        compilerError(token, "Expected 🍇 but found %s instead.", s);
    }
    while (token = consumeToken(IDENTIFIER), token->value[0] != E_WATERMELON) {
        eenum->addValueFor(token->value[0]);
    }
}

void parseClassBody(Class *eclass, Package *pkg, std::vector<Initializer *> *requiredInitializers, bool allowNative) {
    allowNative = allowNative && pkg->requiresBinary();
    
    const Token *token = consumeToken(IDENTIFIER);
    if (token->value[0] != E_GRAPES) {
        ecCharToCharStack(token->value[0], s);
        compilerError(token, "Expected 🍇 but found %s instead.", s);
    }
    while (token = consumeToken(), !(token->type == IDENTIFIER && token->value[0] == E_WATERMELON)) {
        const Token *documentationToken = nullptr;
        if (token->type == DOCUMENTATION_COMMENT) {
            documentationToken = token;
            token = consumeToken();
        }
        token->forceType(IDENTIFIER);
        
        bool deprecated = hasAttribute(E_WARNING_SIGN, &token);
        bool final = hasAttribute(E_LOCK_WITH_INK_PEN, &token);
        AccessLevel accessLevel = readAccessLevel(&token);
        bool override = hasAttribute(E_BLACK_NIB, &token);
        bool staticOnType = hasAttribute(E_RABBIT, &token);
        bool required = hasAttribute(E_KEY, &token);
        bool canReturnNothingness = hasAttribute(E_CANDY, &token);

        switch (token->value[0]) {
            case E_SHORTCAKE: {
                invalidAttribute(staticOnType, E_RABBIT, token);
                invalidAttribute(override, E_LOCK_WITH_INK_PEN, token);
                invalidAttribute(final, E_BLACK_NIB, token);
                invalidAttribute(required, E_KEY, token);
                invalidAttribute(canReturnNothingness, E_CANDY, token);
                invalidAttribute(deprecated, E_WARNING_SIGN, token);
                
                const Token *variableName = consumeToken(VARIABLE);
                
                if (eclass->instanceVariables.size() == 65535) {
                    compilerError(token, "You exceeded the limit of 65,535 instance variables.");
                }

                auto type = Type::parseAndFetchType(Type(eclass), AllowGenericTypeVariables, pkg, nullptr);
                
                eclass->instanceVariables.push_back(new Variable(variableName, type));
            }
            break;
            case E_CROCODILE: {
                invalidAttribute(staticOnType, E_RABBIT, token);
                invalidAttribute(override, E_LOCK_WITH_INK_PEN, token);
                invalidAttribute(final, E_BLACK_NIB, token);
                invalidAttribute(required, E_KEY, token);
                invalidAttribute(canReturnNothingness, E_CANDY, token);
                invalidAttribute(deprecated, E_WARNING_SIGN, token);
                
                Type type = Type::parseAndFetchType(Type(eclass), NoDynamism, pkg, nullptr);
                
                if (type.optional) {
                    compilerError(token, "Please remove 🍬.");
                }
                if (type.type() != TT_PROTOCOL) {
                    compilerError(token, "The given type is not a protocol.");
                }

                eclass->addProtocol(type.protocol);
            }
            break;
            case E_PIG: {
                invalidAttribute(required, E_KEY, token);
                invalidAttribute(canReturnNothingness, E_CANDY, token);
                
                const Token *methodName = consumeToken(IDENTIFIER);
                EmojicodeChar name = methodName->value[0];
                
                if (staticOnType) {
                    auto *classMethod = new ClassMethod(name, accessLevel, final, eclass, pkg,
                                                        token, override, documentationToken, deprecated);
                    classMethod->parseGenericArguments(TypeContext(eclass, classMethod), pkg);
                    classMethod->parseArgumentList(TypeContext(eclass, classMethod), pkg);
                    classMethod->parseReturnType(TypeContext(eclass, classMethod), pkg);
                    classMethod->parseBody(allowNative);
                    
                    if (classMethod->name == E_CHEQUERED_FLAG) {
                        if (foundStartingFlag) {
                            auto className = Type(startingFlag.eclass).toString(typeNothingness, true);
                            compilerError(currentToken,
                                          "Duplicate 🏁 method. Previous 🏁 method was defined in class %s.",
                                          className.c_str());
                        }
                        foundStartingFlag = true;
                        
                        startingFlag.eclass = eclass;
                        startingFlag.method = classMethod;
                        
                        if (!classMethod->returnType.compatibleTo(typeInteger, Type(eclass, false))) {
                            compilerError(methodName, "🏁 method must return 🚂.");
                        }
                    }
                    
                    eclass->addClassMethod(classMethod);
                }
                else {
                    reservedEmojis(methodName, "method");
                    
                    auto *method = new Method(methodName->value[0], accessLevel, final, eclass,
                                              pkg, token, override, documentationToken, deprecated);
                    method->parseGenericArguments(TypeContext(eclass, method), pkg);
                    method->parseArgumentList(TypeContext(eclass, method), pkg);
                    method->parseReturnType(TypeContext(eclass, method), pkg);
                    method->parseBody(allowNative);
                    
                    eclass->addMethod(method);
                }
            }
            break;
            case E_CAT: {
                invalidAttribute(staticOnType, E_RABBIT, token);
                
                const Token *initializerName = consumeToken(IDENTIFIER);
                EmojicodeChar name = initializerName->value[0];
                
                Initializer *initializer = new Initializer(name, accessLevel, final, eclass, pkg, token, override,
                                                           documentationToken, deprecated, required,
                                                           canReturnNothingness);
                initializer->parseArgumentList(TypeContext(eclass, initializer), pkg);
                initializer->parseBody(allowNative);
                
                if (requiredInitializers) {
                    for (size_t i = 0; i < requiredInitializers->size(); i++) {
                        Initializer *c = (*requiredInitializers)[i];
                        if (c->name == initializer->name) {
                            requiredInitializers->erase(requiredInitializers->begin() + i);
                            break;
                        }
                    }
                }
                
                eclass->addInitializer(initializer);
                
                if (required) {
                    eclass->requiredInitializerList.push_back(initializer);
                }
            }
            break;
            default: {
                ecCharToCharStack(token->value[0], cs);
                compilerError(token, "Unexpected identifier %s.", cs);
                break;
            }
        }
    }
}

void parseClass(Package *pkg, const Token *documentationToken, const Token *theToken, bool exported) {
    EmojicodeChar className, enamespace;
    bool optional;
    const Token *classNameToken = Type::parseTypeName(&className, &enamespace, &optional);
    
    checkTypeValidity(className, enamespace, optional, theToken, pkg);
    
    Class *eclass = new Class(className, theToken, pkg, documentationToken);

    while (nextToken()->value[0] == E_SPIRAL_SHELL) {
        consumeToken(IDENTIFIER);
        
        const Token *variable = consumeToken(VARIABLE);
        auto constraintType = Type::parseAndFetchType(Type(eclass), NoDynamism, pkg, nullptr);
        eclass->addGenericArgument(variable, constraintType);
    }
    
    if (nextToken()->value[0] != E_GRAPES) {
        EmojicodeChar typeName, typeNamespace;
        bool optional, existent;
        const Token *token = Type::parseTypeName(&typeName, &typeNamespace, &optional);
        Type type = pkg->fetchRawType(typeName, typeNamespace, optional, token, &existent);
        
        if (!existent) {
            compilerError(token, "Superclass type does not exist.");
        }
        if (type.type() != TT_CLASS) {
            compilerError(token, "The superclass must be a class.");
        }
        if (type.optional) {
            compilerError(classNameToken, "You cannot inherit from an 🍬.");
        }
        
        eclass->superclass = type.eclass;
        
        eclass->setSuperTypeDef(eclass->superclass);
        type.parseGenericArguments(Type(eclass), AllowGenericTypeVariables, pkg, token);
        eclass->setSuperGenericArguments(type.genericArguments);
    }
    else {
        eclass->superclass = nullptr;
        eclass->finalizeGenericArguments();
    }
    
    pkg->registerType(eclass, className, enamespace);
    pkg->registerClass(eclass);
    if (exported) {
        pkg->exportType(eclass, className);
    }
    
    std::vector<Initializer *> requiredInitializers;
    if (eclass->superclass != nullptr) {
        // This list contains methods that must be implemented.
        // If a method is implemented it gets removed from this list by parseClassBody.
        requiredInitializers = std::vector<Initializer *>(eclass->superclass->requiredInitializerList);
    }
    
    eclass->index = classes.size();
    classes.push_back(eclass);
    
    parseClassBody(eclass, pkg, &requiredInitializers, true);
    
    if (requiredInitializers.size()) {
        Initializer *c = requiredInitializers[0];
        ecCharToCharStack(c->name, name);
        compilerError(eclass->classBeginToken(), "Required initializer %s was not implemented.", name);
    }
}

void parseFile(const char *path, Package *pkg) {
    const Token *oldCurrentToken = currentToken;
    
    FILE *in = fopen(path, "rb");
    if (!in || ferror(in)) {
        compilerError(nullptr, "Couldn't read input file %s.", path);
        return;
    }
    
    const char *dot = strrchr(path, '.');
    if (!dot || strcmp(dot, ".emojic")) {
        compilerError(nullptr, "Emojicode files must be suffixed with .emojic: %s", path);
    }
    
    currentToken = lex(in, path);
    
    fclose(in);
    
    for (const Token *theToken = currentToken; theToken != nullptr && theToken->type != NO_TYPE; theToken = consumeToken()) {
        const Token *documentationToken = nullptr;
        if (theToken->type == DOCUMENTATION_COMMENT) {
            documentationToken = theToken;
            theToken = consumeToken(IDENTIFIER);
        }
        
        theToken->forceType(IDENTIFIER);
        
        bool exported = hasAttribute(E_EARTH_GLOBE_EUROPE_AFRICA, &theToken);
        
        switch (theToken->value[0]) {
            case E_PACKAGE: {
                invalidAttribute(exported, E_EARTH_GLOBE_EUROPE_AFRICA, theToken);
                
                const Token *nameToken = consumeToken(VARIABLE);
                const Token *namespaceToken = consumeToken(IDENTIFIER);
                
                auto name = nameToken->value.utf8CString();
                pkg->loadPackage(name, namespaceToken->value[0], theToken);
                
                continue;
            }
            case E_CROCODILE:
                parseProtocol(pkg, documentationToken, exported);
                continue;
            case E_TURKEY:
                parseEnum(pkg, documentationToken, exported);
                continue;
            case E_RADIO:
                invalidAttribute(exported, E_EARTH_GLOBE_EUROPE_AFRICA, theToken);
                pkg->setRequiresBinary();
                if (strcmp(pkg->name(), "_") == 0) {
                    compilerError(theToken, "You may not set 📻 for the _ package.");
                }
                continue;
            case E_CRYSTAL_BALL: {
                invalidAttribute(exported, E_EARTH_GLOBE_EUROPE_AFRICA, theToken);
                if (pkg->version().minor && pkg->version().major) {
                    compilerError(theToken, "Package version already declared.");
                }
                
                const Token *major = consumeToken(INTEGER);
                const Token *minor = consumeToken(INTEGER);
                
                const char *majorString = major->value.utf8CString();
                const char *minorString = minor->value.utf8CString();
                
                uint16_t majori = strtol(majorString, nullptr, 0);
                uint16_t minori = strtol(minorString, nullptr, 0);
                
                delete [] majorString;
                delete [] minorString;
                
                pkg->setPackageVersion(PackageVersion(majori, minori));
                continue;
            }
            case E_WALE: {
                invalidAttribute(exported, E_EARTH_GLOBE_EUROPE_AFRICA, theToken);
                EmojicodeChar className, enamespace;
                bool optional;
                const Token *classNameToken = Type::parseTypeName(&className, &enamespace, &optional);
                
                if (optional) {
                    compilerError(classNameToken, "Optional types are not extendable.");
                }
                
                bool existent;
                Type type = pkg->fetchRawType(className, enamespace, optional, theToken, &existent);
                
                if (!existent) {
                    compilerError(classNameToken, "Class does not exist.");
                }
                if (type.type() != TT_CLASS) {
                    compilerError(classNameToken, "Only classes are extendable.");
                }
                
                auto eclass = type.eclass;
                
                // Native extensions are allowed if the class was defined in this package.
                parseClassBody(eclass, pkg, nullptr, eclass->package() == pkg);
                
                continue;
            }
            case E_RABBIT:
                parseClass(pkg, documentationToken, theToken, exported);
                continue;
            case E_SCROLL: {
                invalidAttribute(exported, E_EARTH_GLOBE_EUROPE_AFRICA, theToken);
                const Token *pathString = consumeToken(STRING);
                
                auto fileString = pathString->value.utf8CString();
                
                char *str = fileString;
                const char *lastSlash = strrchr(path, '/');
                if (lastSlash != nullptr) {
                    const char *directory = strndup(path, lastSlash - path);
                    asprintf(&str, "%s/%s", directory, fileString);
                    delete [] fileString;
                }
                
                parseFile(str, pkg);

                delete [] str;
                continue;
            }
            default:
                ecCharToCharStack(theToken->value[0], f);
                compilerError(theToken, "Unexpected identifier %s", f);
                break;
        }
    }
    currentToken = oldCurrentToken;
}
