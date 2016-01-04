//
//  Reporter.c
//  Emojicode
//
//  Created by Theo Weidmann on 06.11.15.
//  Copyright © 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeCompiler.h"
#include "Procedure.h"
#include <string.h>

typedef enum {
    Return,
    NoReturn,
    CanReturnNothingness
} ReturnManner;

void reportDocumentation(Token *documentationToken) {
    if(!documentationToken) {
        return;
    }
    
    const char *d = documentationToken->value.utf8CString();
    printf("\"documentation\":");
    printJSONStringToFile(d, stdout);
    putc(',', stdout);
    delete d;
}

void reportType(const char *key, Type type, bool last){
    const char *returnTypeName = type.toString(typeNothingness, false);
    
    if (key) {
        printf("\"%s\": {\"package\": \"%s\", \"name\": \"%s\", \"optional\": %s},", key, type.typePackage(), returnTypeName, type.optional ? "true" : "false");
    }
    else {
        printf("{\"package\": \"%s\", \"name\": \"%s\", \"optional\": %s}%s", type.typePackage(), returnTypeName, type.optional ? "true" : "false", last ? "" : ",");
    }
    
    delete returnTypeName;
}

void reportProcedureInformation(Procedure *p, ReturnManner returnm, bool last){
    ecCharToCharStack(p->name, nameString);
    
    printf("{");
    printf("\"name\": \"%s\",", nameString);
    
    if (returnm == Return) {
        reportType("returnType", p->returnType, false);
    }
    else if (returnm == CanReturnNothingness) {
        printf("\"canReturnNothingness\": true,");
    }
    
    reportDocumentation(p->documentationToken);
    
    printf("\"arguments\": [");
    for (int i = 0; i < p->arguments.size(); i++) {
        printf("{");
        Variable variable = p->arguments[i];
        
        const char *varname = variable.name->value.utf8CString();
        
        reportType("type", variable.type, false);
        printf("\"name\":");
        printJSONStringToFile(varname, stdout);
        printf("}%s", i + 1 == p->arguments.size() ? "" : ",");
        
        delete varname;
    }
    printf("]");
    
    printf("}%s", last ? "" : ",");
}

void report(const char *packageName){
    bool printedClass = false;
    printf("{");
    printf("\"classes\": [");
    for(auto eclass : classes){
        if (strcmp(eclass->package->name, packageName) != 0) {
            continue;
        }
        
        if (printedClass) {
            putchar(',');
        }
        printedClass = true;
        
        printf("{");
        
        ecCharToCharStack(eclass->name, className);
        printf("\"name\": \"%s\",", className);
        
        reportDocumentation(eclass->documentationToken);
        
        if (eclass->superclass) {
            ecCharToCharStack(eclass->superclass->name, superClassName);
            printf("\"superclass\": {\"package\": \"%s\", \"name\": \"%s\"},", eclass->superclass->package->name, superClassName);
        }
        
        printf("\"methods\": [");
        for(size_t i = 0; i < eclass->methodList->count; i++){
            Method *method = getList(eclass->methodList, i);
            reportProcedureInformation((Procedure *)method, Return, i + 1 == eclass->methodList->count);
        }
        printf("],");
        
        printf("\"initializers\": [");
        for(size_t i = 0; i < eclass->initializerList->count; i++){
            Initializer *initializer = getList(eclass->initializerList, i);
            reportProcedureInformation((Procedure *)initializer, initializer->canReturnNothingness ? CanReturnNothingness : NoReturn, i + 1 == eclass->initializerList->count);
        }
        printf("],");
        
        printf("\"classMethods\": [");
        for(size_t i = 0; i < eclass->classMethodList->count; i++){
            ClassMethod *classMethod = getList(eclass->classMethodList, i);
            reportProcedureInformation((Procedure *)classMethod, Return, eclass->classMethodList->count == i + 1);
        }
        printf("],");
        
        printf("\"conformsTo\": [");
        for(size_t i = 0; i < eclass->protocols->count; i++){
            Protocol *protocol = getList(eclass->protocols, i);
            reportType(NULL, typeForProtocol(protocol), i + 1 == eclass->protocols->count);
        }
        printf("]}");
    }
    printf("],");
    printf("\"enums\": [");
    bool printedEnum = false;
    for(size_t i = 0; i < enumsRegister->capacity; i++){
        if (!enumsRegister->slots[i].key) {
            continue;
        }
        
        Enum *eenum = enumsRegister->slots[i].value;
        
        if (strcmp(eenum->package->name, packageName) != 0) {
            continue;
        }
        if (printedEnum) {
            putchar(',');
        }
        printf("{");
        
        printedEnum = true;
        
        ecCharToCharStack(eenum->name, enumName);
        printf("\"name\": \"%s\",", enumName);
        
        reportDocumentation(eenum->documentationToken);
        
        bool printedValue = false;
        printf("\"values\": [");
        for(size_t i = 0; i < eenum->dictionary->capacity; i++){
            if (eenum->dictionary->slots[i].key) {
                ecCharToCharStack(*(EmojicodeChar *)eenum->dictionary->slots[i].key, value);
                if (printedValue){
                    putchar(',');
                }
                printf("\"%s\"", value);
                printedValue = true;
            }
        }
        printf("]}");
    }
    printf("],");
    printf("\"protocols\": [");
    bool printedProtocol = false;
    for(size_t i = 0; i < protocolsRegister->capacity; i++){
        if (!protocolsRegister->slots[i].key) {
            continue;
        }
        
        Protocol *protocol = protocolsRegister->slots[i].value;
        
        if (strcmp(protocol->package->name, packageName) != 0) {
            continue;
        }
        if (printedProtocol) {
            putchar(',');
        }
        printedProtocol = true;
        printf("{");
        
        ecCharToCharStack(protocol->name, protocolName);
        printf("\"name\": \"%s\",", protocolName);
        
        reportDocumentation(protocol->documentationToken);
        
        printf("\"methods\": [");
        for(size_t i = 0; i < protocol->methodList->count; i++){
            Method *method = getList(protocol->methodList, i);
            reportProcedureInformation((Procedure *)method, Return, i + 1 == protocol->methodList->count);
        }
        printf("]}");
    }
    printf("]");
    printf("}");
}