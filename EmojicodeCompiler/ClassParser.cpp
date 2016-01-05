//
//  ClassParser.c
//  Emojicode
//
//  Created by Theo Weidmann on 05.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "ClassParser.h"
#include "Lexer.h"
#include "utf8.h"

#include <ctype.h>
#include <libgen.h>
#include <string.h>
#include <limits.h>

void packageRegisterHeaderNewest(const char *name, EmojicodeChar enamespace){
    char *path;
    asprintf(&path, packageDirectory "%s/header.emojic", name);
    
    Package *pkg = new Package(name, (PackageVersion){0, 0}, false);
    
    parseFile(path, pkg, true, enamespace);
    
    if(pkg->version.major == 0 && pkg->version.minor == 0){
        compilerError(NULL, "Package %s does not provide a valid version.", name);
    }
    
    if(packages.size() > 0){
        packages.insert(packages.begin() + packages.size() - 1, pkg);
    }
    else {
        packages.push_back(pkg);
    }
}

//MARK: Tips

/**
 * Use this function to determine if the user has choosen a bad method/initializer name. It puts a warning if a reserved name is used.
 * @param place The place in code (like "method")
 */
void reservedEmojisWarning(Token *token, const char *place){
    EmojicodeChar name = token->value[0];
    switch (name) {
        case E_CUSTARD:
        case E_SHORTCAKE:
        case E_CHOCOLATE_BAR:
        case E_COOKING:
        case E_DOG:
        case E_HIGH_VOLTAGE_SIGN:
        case E_CLOUD:
        case E_BANANA:
        case E_TANGERINE: {
            ecCharToCharStack(name, nameString);
            compilerWarning(token, "Avoid using %s as %s name.", nameString, place);
        }
    }
}


//MARK: Utilities

static Token* until(EmojicodeChar end, EmojicodeChar deeper, int *deep){
    Token *token = consumeToken();
    
    if (token->type != IDENTIFIER) {
        return token;
    }
    
    if(token->value[0] == deeper){
        (*deep)++;
    }
    else if(token->value[0] == end){
        if (*deep == 0){
            return NULL;
        }
        (*deep)--;
    }
    
    return token;
}

/**
 * Parses an argument list from an initializer or method definition and saves it to the @c arguments object.
 */
Arguments parseArgumentList(Type ct, EmojicodeChar enamespace){
    Token *token;
    Arguments arguments;
    
    //Until the grape is found we parse arguments
    while ((token = nextToken()) && token->type == VARIABLE) { //grape
        //Get the variable in which to store the argument
        Token *variableToken = consumeToken();
        variableToken->forceType(VARIABLE);
        
        auto type = parseAndFetchType(ct, enamespace, AllowGenericTypeVariables, NULL);
        
        arguments.push_back(Variable(Variable(variableToken, type)));
    }
    
    if (arguments.size() > UINT8_MAX) {
        compilerError(token, "A function cannot take more than 255 arguments.");
    }
    
    return arguments;
}

Type parseReturnType(Type ct, EmojicodeChar theNamespace){
    if(nextToken()->type == IDENTIFIER && nextToken()->value[0] == E_RIGHTWARDS_ARROW){
        consumeToken();
        return parseAndFetchType(ct, theNamespace, AllowGenericTypeVariables, NULL);
    }
    else {
        return typeNothingness;
    }
}

void saveBlock(Procedure *p, bool allowNative){
    if(nextToken()->value[0] == E_RADIO){
        Token *t = consumeToken();
        if(!allowNative){
            compilerError(t, "Native code is not allowed in this context.");
        }
        p->native = true;
        return;
    }
    
    Token *token = consumeToken();
    
    token->forceType(IDENTIFIER);
    if (token->value[0] != E_GRAPES){
        ecCharToCharStack(token->value[0], c);
        compilerError(token, "Expected 🍇 but found %s instead.", c);
    }
    
    p->firstToken = currentToken;
    
    int d = 0;
    while ((token = until(E_WATERMELON, E_GRAPES, &d)) != NULL);
}

static void checkTypeValidity(EmojicodeChar name, EmojicodeChar enamespace, bool optional, Token *token){
    if(optional){
        compilerError(token, "🍬 cannot be declared as type.");
    }
    bool existent;
    Type type = fetchRawType(name, enamespace, optional, token, &existent);
    if (existent) {
        auto str = type.toString(typeNothingness, true);
        compilerError(currentToken, "Type %s is already defined.", str.c_str());
    }
}

void parseProtocol(EmojicodeChar theNamespace, Package *pkg, Token *documentationToken){
    static uint_fast16_t index = 0;
    
    EmojicodeChar name, enamespace;
    bool optional;
    Token *classNameToken = parseTypeName(&name, &enamespace, &optional, theNamespace);
    
    checkTypeValidity(name, enamespace, optional, classNameToken);
    
    if(index == UINT16_MAX){
        compilerError(classNameToken, "You exceeded the limit of 65,535 protocols.");
    }
    
    auto protocol = new Protocol(name, enamespace, index);
    protocol->documentationToken = documentationToken;
    
    std::array<EmojicodeChar, 2> ns = {enamespace, name};
    protocolsRegister[ns] = protocol;
    
    Token *token = consumeToken();
    token->forceType(IDENTIFIER);
    if (token->value[0] != E_GRAPES){
        ecCharToCharStack(token->value[0], s);
        compilerError(token, "Expected 🍇 but found %s instead.", s);
    }
    while (token = consumeToken(), !(token->type == IDENTIFIER && token->value[0] == E_WATERMELON)) {
        
        Token *documentationToken = NULL;
        if (token->type == DOCUMENTATION_COMMENT) {
            documentationToken = token;
            token = consumeToken();
        }
        
        if (token->value[0] != E_PIG) {
            compilerError(token, "Only method declarations are allowed inside a protocol.");
        }
        
        //Get the method name
        Token *methodName = consumeToken();
        methodName->forceType(IDENTIFIER);
        
        Type returnType = typeNothingness;
        auto method = new Method(methodName->value[0], PUBLIC, false, NULL, theNamespace, methodName, false, documentationToken);
        
        method->duplicateDeclarationCheck<Method *>(protocol->methods);
        
        method->arguments = parseArgumentList(typeNothingness, theNamespace);
        method->returnType = parseReturnType(typeNothingness, theNamespace);
    }
}

void parseEnum(EmojicodeChar theNamespace, Package &pkg, Token *documentationToken){
    EmojicodeChar name, enamespace;
    bool optional;
    Token *enumNameToken = parseTypeName(&name, &enamespace, &optional, theNamespace);
    
    checkTypeValidity(name, theNamespace, optional, enumNameToken);
    
    Enum *eenum = new Enum(name, pkg, documentationToken);
    std::array<EmojicodeChar, 2> ns = {enamespace, name};
    enumsRegister[ns] = eenum;
    
    Token *token = consumeToken();
    token->forceType(IDENTIFIER);
    if (token->value[0] != E_GRAPES) {
        ecCharToCharStack(token->value[0], s);
        compilerError(token, "Expected 🍇 but found %s instead.", s);
    }
    while (token = consumeToken(), !(token->type == IDENTIFIER && token->value[0] == E_WATERMELON)) {
        token->forceType(IDENTIFIER);
        
        eenum->addValueFor(token->value[0]);
    }
}

static bool hasAttribute(EmojicodeChar attributeName, Token **token){
    if((*token)->type == IDENTIFIER && (*token)->value[0] == attributeName){
        *token = consumeToken();
        return true;
    }
    return false;
}

static AccessLevel readAccessLevel(Token **token){
    AccessLevel access;
    (*token)->forceType(IDENTIFIER);
    switch ((*token)->value[0]) {
        case E_CLOSED_LOCK_WITH_KEY:
            *token = consumeToken();
            access = PROTECTED;
            break;
        case E_LOCK:
            *token = consumeToken();
            access = PRIVATE;
            break;
        case E_OPEN_LOCK:
            *token = consumeToken();
        default:
            access = PUBLIC;
    }
    return access;
}

/**
 * Parses a eclass’ body until a 🍆, which it consumes
 * @param eclass The eclass to which to append the methods.
 * @param requiredInitializers Either a list of required initializers or @c NULL (for extensions).
 */
void parseClassBody(Class *eclass, std::vector<Initializer *> *requiredInitializers, bool allowNative, EmojicodeChar theNamespace){
    //Until we find a melon process methods and initializers
    Token *token = consumeToken();
    token->forceType(IDENTIFIER);
    if (token->value[0] != E_GRAPES){
        ecCharToCharStack(token->value[0], s);
        compilerError(token, "Expected 🍇 but found %s instead.", s);
    }
    while (token = consumeToken(), !(token->type == IDENTIFIER && token->value[0] == E_WATERMELON)) {
        
        Token *documentationToken = NULL;
        if (token->type == DOCUMENTATION_COMMENT) {
            documentationToken = token;
            token = consumeToken();
        }
        
        bool final = hasAttribute(E_LOCK_WITH_INK_PEN, &token);
        AccessLevel accessLevel = readAccessLevel(&token);
        bool override = hasAttribute(E_BLACK_NIB, &token);
        bool staticOnType = hasAttribute(E_RABBIT, &token);
        bool required = hasAttribute(E_KEY, &token);
        bool canReturnNothingness = hasAttribute(E_CANDY, &token);

        token->forceType(IDENTIFIER);
        switch (token->value[0]) {
            case E_SHORTCAKE: {
                //Get the variable name
                Token *ivarName = consumeToken();
                ivarName->forceType(VARIABLE);
                
                if(staticOnType){
                    compilerError(ivarName, "Class variables are not supported yet.");
                }
                
                if(eclass->instanceVariables.size() == 65535){
                    compilerError(token, "You exceeded the limit of 65,535 instance variables.");
                }

                auto type = parseAndFetchType(eclass, theNamespace, AllowGenericTypeVariables, NULL);
                
                eclass->instanceVariables.push_back(new Variable(ivarName, type));
            }
            break;
            case E_CROCODILE: {
                if(staticOnType){
                    compilerError(token, "Invalid modifier 🐇.");
                }
                
                Type type = parseAndFetchType(eclass, theNamespace, NoDynamism, NULL);
                if (type.optional) {
                    compilerError(token, "Please remove 🍬.");
                }
                if (type.type != TT_PROTOCOL) {
                    compilerError(token, "The given type is not a protocol.");
                }

                eclass->protocols.push_back(type.protocol);
            }
            break;
            case E_PIG: {
                if(required){
                    compilerError(token, "Invalid modifier 🔑.");
                }
                
                Token *methodName = consumeToken();
                EmojicodeChar name = methodName->value[0];
                methodName->forceType(IDENTIFIER);
                
                if(staticOnType){
                    bool isStartingFlag = false;
                    if(methodName->value[0] == E_CHEQUERED_FLAG){
                        if(foundStartingFlag){
                            ecCharToCharStack(startingFlag.eclass->name, cl);
                            ecCharToCharStack(startingFlag.eclass->enamespace, clnm);
                            compilerError(currentToken, "Duplicate 🏁 method. Previous method was defined in class %s %s.", clnm, cl);
                        }
                        isStartingFlag = true;
                        foundStartingFlag = true;
                    }
                    
                    reservedEmojisWarning(methodName, "class method");
                    
                    auto *classMethod = new ClassMethod(name, accessLevel, final, eclass, theNamespace, token, override, documentationToken);
                    classMethod->duplicateDeclarationCheck<ClassMethod *>(eclass->classMethods);
                    
                    eclass->classMethods.insert(std::map<EmojicodeChar, ClassMethod*>::value_type(name, classMethod));
                    
                    classMethod->arguments = parseArgumentList(eclass, theNamespace);
                    classMethod->returnType = parseReturnType(eclass, theNamespace);
                    
                    //Is this a correct starting flag eclass method?
                    if(isStartingFlag){
                        startingFlag.eclass = eclass;
                        startingFlag.method = classMethod;
                        
                        if(!classMethod->returnType.compatibleTo(typeInteger, Type(eclass, false))){
                            compilerError(methodName, "🏁 method must return 🚂.");
                        }
                    }
                    
                    saveBlock(classMethod, allowNative);
                    eclass->classMethodList.push_back(classMethod);
                }
                else {
                    reservedEmojisWarning(methodName, "method");
                    
                    auto *method = new Method(methodName->value[0], accessLevel, final, eclass, theNamespace, token, override, documentationToken);
                    
                    method->duplicateDeclarationCheck(eclass->methods);
                    eclass->methods.insert(std::map<EmojicodeChar, Method*>::value_type(name, method));

                    method->arguments = parseArgumentList(eclass, theNamespace);
                    method->returnType = parseReturnType(eclass, theNamespace);
                    
                    saveBlock(method, allowNative);
                    eclass->methodList.push_back(method);
                }
            }
            break;
            case E_CAT: {
                if(staticOnType){
                    compilerError(token, "Invalid modifier 🐇.");
                }
                
                Token *initializerName = consumeToken();
                initializerName->forceType(IDENTIFIER);
                EmojicodeChar name = initializerName->value[0];
                
                reservedEmojisWarning(initializerName, "initializer");
                
                Initializer *initializer = new Initializer(name, accessLevel, final, eclass, theNamespace, token, override, documentationToken, required, canReturnNothingness);
                
                initializer->duplicateDeclarationCheck<Initializer *>(eclass->initializers);
                eclass->initializers.insert(std::map<EmojicodeChar, Initializer*>::value_type(name, initializer));
                
                initializer->arguments = parseArgumentList(eclass, theNamespace);
                
                if (requiredInitializers) {
                    for (size_t i = 0; i < requiredInitializers->size(); i++) {
                        Initializer *c = (*requiredInitializers)[i];
                        if(c->name == initializer->name){
                            requiredInitializers->erase(requiredInitializers->begin() + i);
                            break;
                        }
                    }
                }
                
                saveBlock(initializer, allowNative);
                eclass->initializerList.push_back(initializer);
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

void parseClass(EmojicodeChar theNamespace, Package *pkg, bool allowNative, Token *documentationToken, Token *theToken){
    EmojicodeChar className, enamespace;
    bool optional;
    Token *classNameToken = parseTypeName(&className, &enamespace, &optional, theNamespace);
    
    checkTypeValidity(className, enamespace, optional, theToken);
    reservedEmojisWarning(classNameToken, "class");
    
    //Create the eclass
    Class *eclass = new Class;
    eclass->name = className;
    eclass->inheritsContructors = false;
    eclass->enamespace = enamespace;
    eclass->classBegin = theToken;
    eclass->package = pkg;
    eclass->ownGenericArgumentCount = 0;
    eclass->documentationToken = documentationToken;
    
    while (nextToken()->value[0] == E_SPIRAL_SHELL) {
        consumeToken();
        
        Token *variable = consumeToken();
        variable->forceType(VARIABLE);
        
        Type t = parseAndFetchType(eclass, theNamespace, AllowGenericTypeVariables, NULL);
        eclass->genericArgumentContraints.push_back(t);
        
        Type rType(TT_REFERENCE, false);
        rType.reference = eclass->ownGenericArgumentCount;
        
        if (eclass->ownGenericArgumentVariables.count(variable->value)) {
            compilerError(variable, "A generic argument variable with the same name is already in use.");
        }
        eclass->ownGenericArgumentVariables.insert(std::map<EmojicodeString, Type>::value_type(variable->value, rType));
        eclass->ownGenericArgumentCount++;
    }
    
    if (nextToken()->value[0] != E_GRAPES) { //Grape
        EmojicodeChar typeName, typeNamespace;
        bool optional, existent;
        Token *token = parseTypeName(&typeName, &typeNamespace, &optional, theNamespace);
        Type type = fetchRawType(typeName, theNamespace, optional, token, &existent);
        
        if (!existent) {
            compilerError(token, "Superclass type does not exist.");
        }
        if (type.type != TT_CLASS) {
            compilerError(token, "The superclass must be a class.");
        }
        
        eclass->superclass = type.eclass;
        eclass->genericArgumentCount = eclass->ownGenericArgumentCount + eclass->superclass->genericArgumentCount;
        eclass->genericArgumentContraints.insert(eclass->genericArgumentContraints.begin(), eclass->superclass->genericArgumentContraints.begin(), eclass->superclass->genericArgumentContraints.end());
        
        for (auto iterator = eclass->ownGenericArgumentVariables.begin(); iterator != eclass->ownGenericArgumentVariables.end(); iterator++) {
            iterator->second.reference += eclass->superclass->genericArgumentCount;
        }
        
        type.parseGenericArguments(eclass, theNamespace, AllowGenericTypeVariables, token);
        
        if (type.optional){
            compilerError(classNameToken, "Please remove 🍬.");
        }
        if (type.type != TT_CLASS) {
            compilerError(classNameToken, "The type given as superclass is not a class.");
        }
        
        eclass->superGenericArguments = type.genericArguments;
    }
    else {
        eclass->superclass = NULL;
        eclass->genericArgumentCount = eclass->ownGenericArgumentCount;
    }
    
    std::array<EmojicodeChar, 2> ns = {enamespace, className};
    classesRegister[ns] = eclass;
    
    std::vector<Initializer *> requiredInitializers;
    if (eclass->superclass != NULL) {
        //this list contains methods that must be implemented
        requiredInitializers = std::vector<Initializer *>(eclass->superclass->requiredInitializerList);
    }
    
    eclass->index = classes.size();
    classes.push_back(eclass);
    
    parseClassBody(eclass, &requiredInitializers, allowNative, theNamespace);
    
    //The class must be complete in its intial definition
    if (requiredInitializers.size()) {
        Initializer *c = requiredInitializers[0];
        ecCharToCharStack(c->name, name);
        compilerError(eclass->classBegin, "Required initializer %s was not implemented.", name);
    }
}

void parseFile(const char *path, Package *pkg, bool allowNative, EmojicodeChar theNamespace){
    Token *oldCurrentToken = currentToken;
    
    FILE *in = fopen(path, "rb");
    if(!in || ferror(in)){
        compilerError(NULL, "Couldn't read input file %s.", path);
        return;
    }
    
    char *dot = strrchr(path, '.');
    if (!dot || strcmp(dot, ".emojic")){
        compilerError(NULL, "Emojicode files must be suffixed with .emojic: %s", path);
    }
    
    currentToken = lex(in, path);
    
    fclose(in);
    
    bool definedClass = false;
    
    for (Token *theToken = currentToken; theToken != NULL && theToken->type != NO_TYPE; theToken = consumeToken()) {
        Token *documentationToken = NULL;
        if (theToken->type == DOCUMENTATION_COMMENT) {
            documentationToken = theToken;
            theToken = consumeToken();
        }
        
        theToken->forceType(IDENTIFIER);
        
        if (theToken->value[0] == E_PACKAGE) {
            if(definedClass){
                compilerError(theToken, "📦 are only allowed before the first class declaration.");
            }
            
            Token *nameToken = consumeToken();
            Token *namespaceToken = consumeToken();
            
            nameToken->forceType(VARIABLE);
            namespaceToken->forceType(IDENTIFIER);
            
            size_t ds = u8_codingsize(nameToken->value.c_str(), nameToken->value.size());
            //Allocate space for the UTF8 string
            char *name = new char[ds + 1];
            //Convert
            size_t written = u8_toutf8(name, ds, nameToken->value.c_str(), nameToken->value.size());
            name[written] = 0;
            
            packageRegisterHeaderNewest(name, namespaceToken->value[0]);

            continue;
        }
        else if (theToken->value[0] == E_CROCODILE) {
            parseProtocol(theNamespace, pkg, documentationToken);
            continue;
        }
        else if (theToken->value[0] == E_TURKEY) {
            parseEnum(theNamespace, *pkg, documentationToken);
            continue;
        }
        else if (theToken->value[0] == E_RADIO) {
            pkg->requiresNativeBinary = true;
            if (strcmp(pkg->name, "s") == 0 || strcmp(pkg->name, "_") == 0) {
                compilerError(theToken, "You may not set 📻 for the _ package.");
            }
            continue;
        }
        else if (theToken->value[0] == E_CRYSTAL_BALL) {
            if(pkg->version.minor && pkg->version.major){
                compilerError(theToken, "Package version already declared.");
            }
            
            Token *major = consumeToken();
            major->forceType(INTEGER);
            Token *minor = consumeToken();
            minor->forceType(INTEGER);

            const char *majorString = major->value.utf8CString();
            const char *minorString = minor->value.utf8CString();
            
            uint16_t majori = strtol(majorString, NULL, 0);
            uint16_t minori = strtol(minorString, NULL, 0);
            
            delete [] majorString;
            delete [] minorString;
            
            pkg->version = (PackageVersion){majori, minori};
            continue;
        }
        else if (theToken->value[0] == E_WALE) {
            EmojicodeChar className, enamespace;
            bool optional;
            Token *classNameToken = parseTypeName(&className, &enamespace, &optional, theNamespace);
            
            if (optional) {
                compilerError(classNameToken, "Optional types are not extendable.");
            }
            Class *eclass = getClass(className, enamespace);
            if (eclass == NULL) {
                compilerError(classNameToken, "Class does not exist.");
            }
            
            //Native extensions are allowed if the eclass was defined in this package.
            parseClassBody(eclass, NULL, eclass->package == pkg, theNamespace);
            continue;
        }
        else if (theToken->value[0] == E_RABBIT) {
            definedClass = true;
            parseClass(theNamespace, pkg, allowNative, documentationToken, theToken);
        }
        else {
            ecCharToCharStack(theToken->value[0], f);
            compilerError(theToken, "Unexpected identifier %s", f);
        }
        
    }
    currentToken = oldCurrentToken;
}
