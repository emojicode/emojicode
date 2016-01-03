//
//  ClassParser.c
//  Emojicode
//
//  Created by Theo Weidmann on 05.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "ClassParser.h"
#include "utf8.h"
#include <ctype.h>
#include <libgen.h>
#include <string.h>
#include <limits.h>

void packageRegisterHeaderNewest(const char *name, EmojicodeChar namespace){
    char *path;
    asprintf(&path, packageDirectory "%s/header.emojic", name);
    
    Package *pkg = malloc(sizeof(Package));
    pkg->version = (PackageVersion){0, 0};
    pkg->name = name;
    pkg->requiresNativeBinary = false;
    
    parseFile(path, pkg, true, namespace);
    
    if(pkg->version.major == 0 && pkg->version.minor == 0){
        compilerError(NULL, "Package %s does not provide a version.", name);
    }
    
    if(packages->count > 0){
        insertList(packages, pkg, packages->count - 1);
    }
    else {
        appendList(packages, pkg);
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
    tokenTypeCheck(NO_TYPE, token);
    
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
void parseArgumentList(Procedure *p){
    int argsSize = 5;
    
    //An array to hold the arguments
    Variable *variables = malloc(argsSize * sizeof(Variable));
    
    //Setup the arguments element
    p->arguments.count = 0;
    p->arguments.variables = variables;
    
    Token *token;
    
    //Until the grape is found we parse arguments
    while (token = nextToken(), tokenTypeCheck(NO_TYPE, token), token->type == VARIABLE) { //grape
        if(p->arguments.count == argsSize){
            argsSize *= 2;
            p->arguments.variables = realloc(p->arguments.variables, argsSize * sizeof(Variable));
        }
        
        //Get the variable in which to store the argument
        Token *variableToken = consumeToken();
        tokenTypeCheck(VARIABLE, variableToken);
        
        Variable *variable = p->arguments.variables + p->arguments.count;
        variable->name = variableToken;
        variable->type = parseAndFetchType(p->class, p->namespace, AllowGenericTypeVariables, NULL);
        
        p->arguments.count++;
    }
}

void parseReturnType(Type *type, Class *class, EmojicodeChar theNamespace){
    if(nextToken()->type == IDENTIFIER && nextToken()->value[0] == E_RIGHTWARDS_ARROW){
        consumeToken();
        *type = parseAndFetchType(class, theNamespace, AllowGenericTypeVariables, NULL);
    }
    else {
        *type = typeNothingness;
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
    tokenTypeCheck(IDENTIFIER, token);
    if (token->value[0] != E_GRAPES){
        ecCharToCharStack(token->value[0], c);
        compilerError(token, "Expected 🍇 but found %s instead.", c);
    }
    
    p->firstToken = currentToken;
    
    int d = 0;
    while ((token = until(E_WATERMELON, E_GRAPES, &d)) != NULL);
}

void checkPromises(Procedure *sub, Procedure *super, const char *type, Type parentType){
    if(super->final){
        ecCharToCharStack(sub->name, mn);
        compilerError(sub->dToken, "%s of %s was marked 🔏.", type, mn);
    }
    if (!typesCompatible(sub->returnType, super->returnType, parentType)) {
        ecCharToCharStack(sub->name, mn);
        char *supername = typeToString(super->returnType, parentType, true);
        char *this = typeToString(sub->returnType, parentType, true);
        compilerError(sub->dToken, "Return type %s of %s is not compatible with the return type %s of its %s.", this, mn, supername, type);
    }
    if (sub->arguments.count > 0) {
        ecCharToCharStack(sub->name, mn);
        compilerError(sub->dToken, "%s expects arguments but its %s doesn't.", mn, type);
    }
    if(super->arguments.count != sub->arguments.count){
        ecCharToCharStack(sub->name, mn);
        compilerError(sub->dToken, "%s expects %s arguments than its %s.", mn, (super->arguments.count < sub->arguments.count) ? "more" : "less", type);
    }
    for (uint8_t i = 0; i < super->arguments.count; i++) {
        //other way, because the method may define a more generic type
        if (!typesCompatible(super->arguments.variables[i].type, sub->arguments.variables[i].type, parentType)) {
            char *supertype = typeToString(super->arguments.variables[i].type, parentType, true);
            char *this = typeToString(sub->arguments.variables[i].type, parentType, true);
            compilerError(sub->dToken, "Type %s of argument %d is not compatible with its %s argument type %s.", this, i + 1, type, supertype);
        }
    }
}

static void checkTypeValidity(EmojicodeChar name, EmojicodeChar namespace, bool optional, Token *token){
    if(optional){
        compilerError(token, "🍬 cannot be declared as type.");
    }
    bool existent;
    Type type = fetchRawType(name, namespace, optional, token, &existent);
    if (existent) {
        char *str = typeToString(type, typeNothingness, true);
        compilerError(currentToken, "Type %s is already defined.", str);
    }
}

void parseProtocol(EmojicodeChar theNamespace, Package *pkg, Token *documentationToken){
    static uint_fast16_t index = 0;
    
    EmojicodeChar name, namespace;
    bool optional;
    Token *classNameToken = parseTypeName(&name, &namespace, &optional, theNamespace);
    
    checkTypeValidity(name, namespace, optional, classNameToken);
    
    if(index == UINT16_MAX){
        compilerError(classNameToken, "You exceeded the limit of 65,535 protocols.");
    }
    
    Protocol *protocol = newProtocol(name, namespace);
    protocol->index = index++;
    protocol->package = pkg;
    protocol->documentationToken = documentationToken;
    
    Token *token = consumeToken();
    tokenTypeCheck(IDENTIFIER, token);
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
        
        Type returnType = typeNothingness;
        Arguments arguments;
        
        //Get the method name
        Token *methodName = consumeToken();
        tokenTypeCheck(IDENTIFIER, methodName);
        
        for (size_t i = 0; i < protocol->methodList->count; i++) {
            
            Method *aMethod = getList(protocol->methodList, i);
            if (aMethod->pc.name == methodName->value[0]) {
                ecCharToCharStack(methodName->value[0], mn);
                ecCharToCharStack(name, cl);
                ecCharToCharStack(namespace, ns);
                compilerError(token, "Method %s is declared twice in protocol %s %s.", mn, cl, ns);
            }
            
        }
        
        Method *method = protocolAddMethod(methodName->value[0], protocol, arguments, returnType);
        method->pc.documentationToken = documentationToken;
        
        parseArgumentList((Procedure *)method);
        parseReturnType(&method->pc.returnType, NULL, theNamespace);
    }
}

void parseEnum(EmojicodeChar theNamespace, Package *pkg, Token *documentationToken){
    EmojicodeChar name, namespace;
    bool optional;
    Token *enumNameToken = parseTypeName(&name, &namespace, &optional, theNamespace);
    
    checkTypeValidity(name, namespace, optional, enumNameToken);
    
    Enum *eenum = newEnum(name, namespace);
    EmojicodeInteger v = 0;
    eenum->package = pkg;
    eenum->documentationToken = documentationToken;
    
    Token *token = consumeToken();
    tokenTypeCheck(IDENTIFIER, token);
    if (token->value[0] != E_GRAPES) {
        ecCharToCharStack(token->value[0], s);
        compilerError(token, "Expected 🍇 but found %s instead.", s);
    }
    while (token = consumeToken(), !(token->type == IDENTIFIER && token->value[0] == E_WATERMELON)) {
        tokenTypeCheck(IDENTIFIER, token);
        
        enumAddValue(token->value[0], eenum, v++);
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
    tokenTypeCheck(IDENTIFIER, *token);
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

static void setProcedure(Procedure *pc, EmojicodeChar name, AccessLevel level, bool final, Class *class, EmojicodeChar theNamespace, Token *dToken, bool overriding, Token *documentationToken){
    pc->name = name;
    pc->native = false;
    pc->access = level;
    pc->final = final;
    pc->class = class;
    pc->dToken = dToken;
    pc->namespace = theNamespace;
    pc->overriding = overriding;
    pc->documentationToken = documentationToken;
}

static void duplicateDeclarationCheck(Procedure *pc, const char *on, Dictionary *dict){
    if (dictionaryLookup(dict, &pc->name, sizeof(pc->name))) {
        ecCharToCharStack(pc->name, name);
        compilerError(pc->dToken, "%s %s is declared twice.", on, name);
    }
}

/**
 * Parses a class’ body until a 🍆, which it consumes
 * @param class The class to which to append the methods.
 * @param requiredInitializers Either a list of required initializers or @c NULL (for extensions).
 */
void parseClassBody(Class *class, List *requiredInitializers, bool allowNative, EmojicodeChar theNamespace){
    //Until we find a melon process methods and initializers
    Token *token = consumeToken();
    tokenTypeCheck(IDENTIFIER, token);
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

        tokenTypeCheck(IDENTIFIER, token);
        switch (token->value[0]) {
            case E_SHORTCAKE: {
                //Get the variable name
                Token *ivarName = consumeToken();
                tokenTypeCheck(VARIABLE, ivarName);
                
                if(staticOnType){
                    compilerError(ivarName, "Class variables are not supported yet.");
                }
                
                //Check there is enough space
                if(class->instanceVariableCount == class->instanceVariablesSize){
                    int x = class->instanceVariablesSize;
                    if(x == 65535){
                        compilerError(token, "You exceeded the limit of 65,535 instance variables.");
                    }
                    else if(x * 2 > 65535){
                        //We do not want to risk an overflow
                        class->instanceVariablesSize = 65535;
                    }
                    else {
                        class->instanceVariablesSize *= 2;
                    }
                    class->instanceVariables = realloc(class->instanceVariables, class->instanceVariablesSize * sizeof(Variable*));
                }

                Variable *ivar = malloc(sizeof(Variable));
                ivar->name = ivarName;
                ivar->type = parseAndFetchType(class, theNamespace, AllowGenericTypeVariables, NULL);
                
                class->instanceVariables[class->instanceVariableCount] = ivar;
                class->instanceVariableCount++;
            }
            break;
            case E_CROCODILE: {
                if(staticOnType){
                    compilerError(token, "Invalid modifier 🐇.");
                }
                
                Type type = parseAndFetchType(class, theNamespace, NoDynamism, NULL);
                if (type.optional) {
                    compilerError(token, "Please remove 🍬.");
                }
                if (type.type != TT_PROTOCOL) {
                    compilerError(token, "The given type is not a protocol.");
                }

                appendList(class->protocols, type.protocol);
            }
            break;
            case E_PIG: {
                if(required){
                    compilerError(token, "Invalid modifier 🔑.");
                }
                
                Token *methodName = consumeToken();
                tokenTypeCheck(IDENTIFIER, methodName);
                
                if(staticOnType){
                    bool isStartingFlag = false;
                    if(methodName->value[0] == E_CHEQUERED_FLAG){
                        if(foundStartingFlag){
                            ecCharToCharStack(startingFlag.class->name, cl);
                            ecCharToCharStack(startingFlag.class->namespace, clnm);
                            compilerError(currentToken, "Duplicate 🏁 method. Previous method was defined in class %s %s.", clnm, cl);
                        }
                        isStartingFlag = true;
                        foundStartingFlag = true;
                    }
                    
                    reservedEmojisWarning(methodName, "class method");
                    
                    ClassMethod *classMethod = malloc(sizeof(ClassMethod));
                    setProcedure((Procedure *)classMethod, methodName->value[0], accessLevel, final, class, theNamespace, token, override, documentationToken);
                    
                    duplicateDeclarationCheck((Procedure *)classMethod, "Class method", class->classMethods);
                    dictionarySet(class->classMethods, &methodName->value[0], sizeof(EmojicodeChar), classMethod);
                    parseArgumentList((Procedure *)classMethod);
                    parseReturnType(&classMethod->pc.returnType, class, theNamespace);
                    
                    //Is this a correct starting flag class method?
                    if(isStartingFlag){
                        startingFlag.class = class;
                        startingFlag.method = classMethod;
                        
                        if(!typesCompatible(classMethod->pc.returnType, typeInteger, typeForClass(class))){
                            compilerError(methodName, "🏁 method must return 🚂.");
                        }
                    }
                    
                    saveBlock((Procedure *)classMethod, allowNative);
                    appendList(class->classMethodList, classMethod);
                }
                else {
                    reservedEmojisWarning(methodName, "method");
                    
                    Method *method = malloc(sizeof(Method));
                    setProcedure(&method->pc, methodName->value[0], accessLevel, final, class, theNamespace, token, override, documentationToken);
                    duplicateDeclarationCheck((Procedure *)method, "Method", class->methods);
                    dictionarySet(class->methods, &methodName->value[0], sizeof(EmojicodeChar), method);

                    parseArgumentList((Procedure *)method);
                    parseReturnType(&method->pc.returnType, class, theNamespace);
                    
                    saveBlock((Procedure *)method, allowNative);
                    appendList(class->methodList, method);
                }
            }
            break;
            case E_CAT: {
                if(staticOnType){
                    compilerError(token, "Invalid modifier 🐇.");
                }
                
                Token *initializerName = consumeToken();
                tokenTypeCheck(IDENTIFIER, initializerName);
                
                reservedEmojisWarning(initializerName, "initializer");
                
                Initializer *initializer = malloc(sizeof(Initializer));
                setProcedure((Procedure *)initializer, initializerName->value[0], accessLevel, final, class, theNamespace, token, override, documentationToken);
                initializer->required = required;
                initializer->canReturnNothingness = canReturnNothingness;
                
                duplicateDeclarationCheck((Procedure *)initializer, "Initializer", class->initializers);
                dictionarySet(class->initializers, &initializerName->value[0], sizeof(EmojicodeChar), initializer);
                
                parseArgumentList((Procedure *)initializer);
                
                if(requiredInitializers){
                    for (size_t i = 0; i < requiredInitializers->count; i++) {
                        Initializer *c = getList(requiredInitializers, i);
                        if(c->pc.name == initializer->pc.name){
                            listRemoveByIndex(requiredInitializers, i);
                            break;
                        }
                    }
                }
                
                initializer->pc.returnType = typeNothingness;
                saveBlock((Procedure *)initializer, allowNative);
                appendList(class->initializerList, initializer);
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
    EmojicodeChar className, namespace;
    bool optional;
    Token *classNameToken = parseTypeName(&className, &namespace, &optional, theNamespace);
    
    checkTypeValidity(className, namespace, optional, theToken);
    reservedEmojisWarning(classNameToken, "class");
    
    //Create the class
    Class *class = malloc(sizeof(Class));
    class->name = className;
    class->methods = newDictionary();
    class->initializers = newDictionary();
    class->classMethods = newDictionary();
    class->instanceVariables = NULL;
    class->instanceVariableCount = 0;
    class->inheritsContructors = false;
    class->namespace = namespace;
    class->requiredInitializerList = newList();
    class->protocols = newList();
    class->methodList = newList();
    class->initializerList = newList();
    class->classMethodList = newList();
    class->classBegin = theToken;
    class->package = pkg;
    class->ownGenericArgumentCount = 0;
    class->genericArgumentContraints = NULL;
    class->ownGenericArgumentVariables = NULL;
    class->documentationToken = documentationToken;
    
    while (nextToken()->value[0] == E_SPIRAL_SHELL) {
        Token *token = consumeToken();
        
        //TODO: Use list?
        if(class->ownGenericArgumentCount == 0){
            class->genericArgumentContraints = malloc(sizeof(Type) * 5);
            class->ownGenericArgumentVariables = newDictionary();
        }
        else if (class->ownGenericArgumentCount == 5){
            compilerError(token, "A class may take up to 5 generic arguments at most.");
        }
        
        Token *variable = consumeToken();
        tokenTypeCheck(VARIABLE, variable);
        
        Type t = parseAndFetchType(class, theNamespace, AllowGenericTypeVariables, NULL);
        class->genericArgumentContraints[class->ownGenericArgumentCount] = t;
        
        Type *rType = malloc(sizeof(Type));
        rType->optional = false;
        rType->reference = class->ownGenericArgumentCount;
        rType->type = TT_REFERENCE;
        
        if (dictionaryLookup(class->ownGenericArgumentVariables, variable->value, variable->valueLength)) {
            compilerError(variable, "A generic argument variable with the same name is already in use.");
        }
        dictionarySet(class->ownGenericArgumentVariables, variable->value, variable->valueLength * sizeof(EmojicodeChar), rType);
        class->ownGenericArgumentCount++;
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
        
        class->superclass = type.class;
        class->genericArgumentCount = class->ownGenericArgumentCount + class->superclass->genericArgumentCount;
        
        Type *genericArgumentContraints = malloc(sizeof(Type) * class->genericArgumentCount);
        memcpy(genericArgumentContraints, class->superclass->genericArgumentContraints, class->superclass->genericArgumentCount * sizeof(Type));
        memcpy(genericArgumentContraints + class->superclass->genericArgumentCount, class->genericArgumentContraints, class->ownGenericArgumentCount * sizeof(Type));
        
        free(class->genericArgumentContraints);
        class->genericArgumentContraints = genericArgumentContraints;
        
        if (class->ownGenericArgumentCount) {
            for (size_t i = 0; i < class->ownGenericArgumentVariables->capacity; i++) {
                if(class->ownGenericArgumentVariables->slots[i].key){
                    Type *rType = class->ownGenericArgumentVariables->slots[i].value;
                    rType->reference += class->superclass->genericArgumentCount;
                }
            }
        }
        
        int offset = initializeAndCopySuperGenericArguments(&type);
        if (offset >= 0) {
            int i = 0;
            while(nextToken()->value[0] == 0x1F41A){
                Token *token = consumeToken();
                
                Type ta = parseAndFetchType(class, theNamespace, AllowGenericTypeVariables, NULL);
                validateGenericArgument(ta, i, type, token);
                type.genericArguments[offset + i] = ta;
                
                i++;
            }
            checkEnoughGenericArguments(i, type, token);
        }
        
        if (type.optional){
            compilerError(classNameToken, "Please remove 🍬.");
        }
        if (type.type != TT_CLASS) {
            compilerError(classNameToken, "The type given as superclass is not a class.");
        }
        
        class->superGenericArguments = type.genericArguments;
    }
    else {
        class->superclass = NULL;
        class->genericArgumentCount = class->ownGenericArgumentCount;
    }
    
    EmojicodeChar ns[2] = {namespace, className};
    dictionarySet(classesRegister, &ns, sizeof(ns), class);
    
    List *requiredInitializers;
    if(class->superclass == NULL){
        requiredInitializers = newList();
    }
    else {
        //this list contains methods that must be implemented
        requiredInitializers = listFromList(class->superclass->requiredInitializerList);
    }
    
    //Allocate space for the instance variable
    class->instanceVariables = malloc(5 * sizeof(Variable*));
    class->instanceVariableCount = 0;
    class->instanceVariablesSize = 5;
    
    class->index = classes->count;
    
    appendList(classes, class);
    
    parseClassBody(class, requiredInitializers, allowNative, theNamespace);
    
    //The class must be complete in its intial definition
    if (requiredInitializers->count) {
        Initializer *c = getList(requiredInitializers, 0);
        ecCharToCharStack(c->pc.name, name);
        compilerError(class->classBegin, "Required initializer %s was not implemented.", name);
    }
    
    listRelease(requiredInitializers);
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
        
        tokenTypeCheck(IDENTIFIER, theToken);
        
        if (theToken->value[0] == E_PACKAGE) {
            if(definedClass){
                compilerError(theToken, "📦 are only allowed before the first class declaration.");
            }
            
            Token *nameToken = consumeToken();
            Token *namespaceToken = consumeToken();
            
            tokenTypeCheck(VARIABLE, nameToken);
            tokenTypeCheck(IDENTIFIER, namespaceToken);
            
            size_t ds = u8_codingsize(nameToken->value, nameToken->valueLength);
            //Allocate space for the UTF8 string
            char *name = malloc(ds + 1);
            //Convert
            size_t written = u8_toutf8(name, ds, nameToken->value, nameToken->valueLength);
            name[written] = 0;
            
            packageRegisterHeaderNewest(name, namespaceToken->value[0]);

            continue;
        }
        else if (theToken->value[0] == E_CROCODILE) {
            parseProtocol(theNamespace, pkg, documentationToken);
            continue;
        }
        else if (theToken->value[0] == E_TURKEY) {
            parseEnum(theNamespace, pkg, documentationToken);
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
            tokenTypeCheck(INTEGER, major);
            Token *minor = consumeToken();
            tokenTypeCheck(INTEGER, minor);
            
            char mi[major->valueLength + 1];
            for(uint32_t i = 0; i < major->valueLength; i++){
                mi[i] = major->value[i];
            }
            mi[major->valueLength] = 0;
            
            char mii[minor->valueLength + 1];
            for(uint32_t i = 0; i < minor->valueLength; i++){
                mii[i] = minor->value[i];
            }
            mii[major->valueLength] = 0;
            
            uint16_t majori = strtol(mi, NULL, 0);
            uint16_t minori = strtol(mii, NULL, 0);
            
            pkg->version = (PackageVersion){majori, minori};
            continue;
        }
        else if (theToken->value[0] == E_WALE) {
            EmojicodeChar className, namespace;
            bool optional;
            Token *classNameToken = parseTypeName(&className, &namespace, &optional, theNamespace);
            
            if (optional) {
                compilerError(classNameToken, "Optional types are not extendable.");
            }
            Class *class = getClass(className, namespace);
            if (class == NULL) {
                compilerError(classNameToken, "Class does not exist.");
            }
            
            //Native extensions are allowed if the class was defined in this package.
            parseClassBody(class, NULL, class->package == pkg, theNamespace);
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
