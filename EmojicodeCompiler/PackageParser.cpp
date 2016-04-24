//
//  PackageParser.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 24/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "PackageParser.hpp"
#include "Class.hpp"
#include "Procedure.hpp"
#include <cstring>

void PackageParser::reservedEmojis(const Token *token, const char *place) const {
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

const Token* PackageParser::parseAndValidateTypeName(EmojicodeChar *name, EmojicodeChar *ns) {
    bool optional;
    const Token *nameToken = Type::parseTypeName(name, ns, &optional);
    
    if (optional) {
        compilerError(nameToken, "🍬 cannot be declared as type.");
    }
    
    Type type = typeNothingness;
    if (package_->fetchRawType(*name, *ns, optional, nameToken, &type)) {
        auto str = type.toString(typeNothingness, true);
        compilerError(currentToken, "Type %s is already defined.", str.c_str());
    }
    
    return nameToken;
}

void PackageParser::parseGenericArgumentList(TypeDefinitionWithGenerics *typeDef, TypeContext tc) {
    while (nextToken()->value[0] == E_SPIRAL_SHELL) {
        consumeToken(IDENTIFIER);
        
        const Token *variable = consumeToken(VARIABLE);
        auto constraintType = Type::parseAndFetchType(tc, NoDynamism, package_, nullptr, true);
        typeDef->addGenericArgument(variable, constraintType);
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

void PackageParser::parseProtocol(const Token *documentationToken, bool exported) {
    EmojicodeChar name, enamespace;
    parseAndValidateTypeName(&name, &enamespace);
    
    auto protocol = new Protocol(name, package_, documentationToken);
    
    parseGenericArgumentList(protocol, Type(protocol, false));
    protocol->finalizeGenericArguments();
    
    auto token = consumeToken(IDENTIFIER);
    if (token->value[0] != E_GRAPES) {
        ecCharToCharStack(token->value[0], s);
        compilerError(token, "Expected 🍇 but found %s instead.", s);
    }
    
    auto protocolType = Type(protocol, false);
    package_->registerType(protocolType, name, enamespace, exported);
    
    while (token = consumeToken(), !(token->type == IDENTIFIER && token->value[0] == E_WATERMELON)) {
        const Token *documentationToken = nullptr;
        if (token->type == DOCUMENTATION_COMMENT) {
            documentationToken = token;
            token = consumeToken();
        }
        token->forceType(IDENTIFIER);
        
        auto deprecated = Attribute<E_WARNING_SIGN>().parse(&token);
        
        if (token->value[0] != E_PIG) {
            compilerError(token, "Only method declarations are allowed inside a protocol.");
        }
        
        auto methodName = consumeToken(IDENTIFIER);
        
        auto method = new Method(methodName->value[0], PUBLIC, false, nullptr, package_, methodName,
                                 false, documentationToken, deprecated.set());
        auto a = method->parseArgumentList(protocolType, package_);
        auto b = method->parseReturnType(protocolType, package_);
        if (a || b) {
            protocol->setUsesSelf();
        }
        
        protocol->addMethod(method);
    }
}

void PackageParser::parseEnum(const Token *documentationToken, bool exported) {
    EmojicodeChar name, enamespace;
    parseAndValidateTypeName(&name, &enamespace);
    
    Enum *eenum = new Enum(name, package_, documentationToken);
    
    package_->registerType(Type(eenum, false), name, enamespace, exported);
    
    const Token *token = consumeToken(IDENTIFIER);
    if (token->value[0] != E_GRAPES) {
        ecCharToCharStack(token->value[0], s);
        compilerError(token, "Expected 🍇 but found %s instead.", s);
    }
    while (token = consumeToken(IDENTIFIER), token->value[0] != E_WATERMELON) {
        eenum->addValueFor(token->value[0]);
    }
}

void PackageParser::parseClassBody(Class *eclass, std::set<EmojicodeChar> *requiredInitializers, bool allowNative) {
    allowNative = allowNative && package_->requiresBinary();
    
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
        
        auto deprecated = Attribute<E_WARNING_SIGN>().parse(&token);
        auto final = Attribute<E_LOCK_WITH_INK_PEN>().parse(&token);
        AccessLevel accessLevel = readAccessLevel(&token);
        auto override = Attribute<E_BLACK_NIB>().parse(&token);
        auto staticOnType = Attribute<E_RABBIT>().parse(&token);
        auto required = Attribute<E_KEY>().parse(&token);
        auto canReturnNothingness = Attribute<E_CANDY>().parse(&token);
        
        switch (token->value[0]) {
            case E_SHORTCAKE: {
                staticOnType.disallow();
                override.disallow();
                final.disallow();
                required.disallow();
                canReturnNothingness.disallow();
                deprecated.disallow();

                const Token *variableName = consumeToken(VARIABLE);
                
                if (eclass->instanceVariables.size() == 65536) {
                    compilerError(token, "You exceeded the limit of 65,536 instance variables.");
                }
                
                auto type = Type::parseAndFetchType(Type(eclass), GenericTypeVariables, package_, nullptr);
                
                eclass->instanceVariables.push_back(new Variable(variableName, type));
            }
                break;
            case E_CROCODILE: {
                staticOnType.disallow();
                override.disallow();
                final.disallow();
                required.disallow();
                canReturnNothingness.disallow();
                deprecated.disallow();
                
                Type type = Type::parseAndFetchType(Type(eclass), GenericTypeVariables, package_, nullptr, true);
                
                if (type.optional()) {
                    compilerError(token, "A class cannot conform to an 🍬 protocol.");
                }
                if (type.type() != TT_PROTOCOL) {
                    compilerError(token, "The given type is not a protocol.");
                }
                
                eclass->addProtocol(type);
            }
                break;
            case E_PIG: {
                required.disallow();
                canReturnNothingness.disallow();

                const Token *methodName = consumeToken(IDENTIFIER);
                EmojicodeChar name = methodName->value[0];
                
                if (staticOnType.set()) {
                    auto *classMethod = new ClassMethod(name, accessLevel, final.set(), eclass, package_,
                                                        token, override.set(), documentationToken, deprecated.set());
                    classMethod->parseGenericArguments(TypeContext(eclass, classMethod), package_);
                    classMethod->parseArgumentList(TypeContext(eclass, classMethod), package_);
                    classMethod->parseReturnType(TypeContext(eclass, classMethod), package_);
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
                    
                    auto *method = new Method(methodName->value[0], accessLevel, final.set(), eclass,
                                              package_, token, override.set(), documentationToken, deprecated.set());
                    method->parseGenericArguments(TypeContext(eclass, method), package_);
                    method->parseArgumentList(TypeContext(eclass, method), package_);
                    method->parseReturnType(TypeContext(eclass, method), package_);
                    method->parseBody(allowNative);
                    
                    eclass->addMethod(method);
                }
            }
                break;
            case E_CAT: {
                staticOnType.disallow();
                
                const Token *initializerName = consumeToken(IDENTIFIER);
                EmojicodeChar name = initializerName->value[0];
                
                Initializer *initializer = new Initializer(name, accessLevel, final.set(), eclass, package_, token,
                                                           override.set(), documentationToken, deprecated.set(),
                                                           required.set(), canReturnNothingness.set());
                initializer->parseArgumentList(TypeContext(eclass, initializer), package_);
                initializer->parseBody(allowNative);
                
                if (requiredInitializers) {
                    requiredInitializers->erase(name);
                }
                
                eclass->addInitializer(initializer);
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

void PackageParser::parseClass(const Token *documentationToken, const Token *theToken, bool exported) {
    EmojicodeChar name, enamespace;
    parseAndValidateTypeName(&name, &enamespace);
    
    auto eclass = new Class(name, theToken, package_, documentationToken);
    
    parseGenericArgumentList(eclass, Type(eclass));
    
    if (nextToken()->value[0] != E_GRAPES) {
        EmojicodeChar typeName, typeNamespace;
        bool optional;
        const Token *token = Type::parseTypeName(&typeName, &typeNamespace, &optional);
        
        Type type = typeNothingness;
        if (!package_->fetchRawType(typeName, typeNamespace, optional, token, &type)) {
            compilerError(token, "Superclass type does not exist.");
        }
        if (type.type() != TT_CLASS) {
            compilerError(token, "The superclass must be a class.");
        }
        if (type.optional()) {
            compilerError(token, "You cannot inherit from an 🍬.");
        }
        
        eclass->superclass = type.eclass;
        
        eclass->setSuperTypeDef(eclass->superclass);
        type.parseGenericArguments(Type(eclass), GenericTypeVariables, package_, token);
        eclass->setSuperGenericArguments(type.genericArguments);
    }
    else {
        eclass->superclass = nullptr;
        eclass->finalizeGenericArguments();
    }
    
    package_->registerType(eclass, name, enamespace, exported);
    package_->registerClass(eclass);
    
    std::set<EmojicodeChar> requiredInitializers;
    if (eclass->superclass != nullptr) {
        // This set contains methods that must be implemented.
        // If a method is implemented it gets removed from this list by parseClassBody.
        requiredInitializers = std::set<EmojicodeChar>(eclass->superclass->requiredInitializers());
    }
    
    parseClassBody(eclass, &requiredInitializers, true);
    
    if (requiredInitializers.size()) {
        ecCharToCharStack(*requiredInitializers.begin(), name);
        compilerError(eclass->classBeginToken(), "Required initializer %s was not implemented.", name);
    }
}

void PackageParser::parse(const char *path) {
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
        
        auto exported = Attribute<E_EARTH_GLOBE_EUROPE_AFRICA>().parse(&theToken);
        
        switch (theToken->value[0]) {
            case E_PACKAGE: {
                exported.disallow();
                
                const Token *nameToken = consumeToken(VARIABLE);
                const Token *namespaceToken = consumeToken(IDENTIFIER);
                
                auto name = nameToken->value.utf8CString();
                package_->loadPackage(name, namespaceToken->value[0], theToken);
                
                continue;
            }
            case E_CROCODILE:
                parseProtocol(documentationToken, exported.set());
                continue;
            case E_TURKEY:
                parseEnum(documentationToken, exported.set());
                continue;
            case E_RADIO:
                exported.disallow();
                package_->setRequiresBinary();
                if (strcmp(package_->name(), "_") == 0) {
                    compilerError(theToken, "You may not set 📻 for the _ package.");
                }
                continue;
            case E_CRYSTAL_BALL: {
                exported.disallow();
                if (package_->validVersion()) {
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
                
                package_->setPackageVersion(PackageVersion(majori, minori));
                if (!package_->validVersion()) {
                    compilerError(theToken, "The provided package version is not valid.");
                }
                
                continue;
            }
            case E_WALE: {
                exported.disallow();
                EmojicodeChar className, enamespace;
                bool optional;
                const Token *classNameToken = Type::parseTypeName(&className, &enamespace, &optional);
                
                if (optional) {
                    compilerError(classNameToken, "Optional types are not extendable.");
                }
                
                Type type = typeNothingness;
                
                if (!package_->fetchRawType(className, enamespace, optional, theToken, &type)) {
                    compilerError(classNameToken, "Class does not exist.");
                }
                if (type.type() != TT_CLASS) {
                    compilerError(classNameToken, "Only classes are extendable.");
                }
                
                // Native extensions are allowed if the class was defined in this package.
                parseClassBody(type.eclass, nullptr, type.eclass->package() == package_);
                
                continue;
            }
            case E_RABBIT:
                parseClass(documentationToken, theToken, exported.set());
                continue;
            case E_SCROLL: {
                exported.disallow();
                const Token *pathString = consumeToken(STRING);
                
                auto fileString = pathString->value.utf8CString();
                
                char *str = fileString;
                const char *lastSlash = strrchr(path, '/');
                if (lastSlash != nullptr) {
                    const char *directory = strndup(path, lastSlash - path);
                    asprintf(&str, "%s/%s", directory, fileString);
                    delete [] fileString;
                }
                
                parse(str);
                
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
