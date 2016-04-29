//
//  Reporter.c
//  Emojicode
//
//  Created by Theo Weidmann on 06.11.15.
//  Copyright © 2015 Theo Weidmann. All rights reserved.
//

#include <cstring>
#include <vector>
#include <list>
#include <map>
#include "utf8.h"
#include "Lexer.hpp"
#include "Procedure.hpp"
#include "Class.hpp"
#include "EmojicodeCompiler.hpp"
#include "Enum.hpp"
#include "Protocol.hpp"

enum ReturnManner {
    Return,
    NoReturn,
    CanReturnNothingness
};

void reportDocumentation(const EmojicodeString &documentation) {
    if (documentation.size() == 0) {
        return;
    }
    
    const char *d = documentation.utf8CString();
    printf("\"documentation\":");
    printJSONStringToFile(d, stdout);
    putc(',', stdout);
    delete [] d;
}

void reportType(const char *key, Type type, TypeContext tc) {
    auto returnTypeName = type.toString(tc, false).c_str();
    
    if (key) {
        printf("\"%s\": {\"package\": \"%s\", \"name\": \"%s\", \"optional\": %s}",
               key, type.typePackage(), returnTypeName, type.optional() ? "true" : "false");
    }
    else {
        printf("{\"package\": \"%s\", \"name\": \"%s\", \"optional\": %s}",
               type.typePackage(), returnTypeName, type.optional() ? "true" : "false");
    }
}

void commaPrinter(bool *b) {
    if (*b) {
        putc(',', stdout);
    }
    *b = true;
}

void reportGenericArguments(std::map<EmojicodeString, Type> map, std::vector<Type> constraints,
                            size_t superCount, TypeContext tc) {
    printf("\"genericArguments\": [");
    
    auto gans = std::vector<EmojicodeString>(map.size());
    for (auto it : map) {
        gans[it.second.reference - superCount] = it.first;
    }
    
    auto reported = false;
    
    for (size_t i = 0; i < gans.size(); i++) {
        auto gan = gans[i];
        auto utf8 = gan.utf8CString();
        commaPrinter(&reported);
        printf("{\"name\": ");
        printJSONStringToFile(utf8, stdout);
        printf(",");
        reportType("constraint", constraints[i], tc);
        printf("}");
        delete [] utf8;
    }
    
    printf("],");
}

void reportProcedureInformation(Procedure *p, ReturnManner returnm, bool last, TypeContext tc) {
    ecCharToCharStack(p->name, nameString);
    
    printf("{");
    printf("\"name\": \"%s\",", nameString);
    if (p->access == PRIVATE) {
        printf("\"access\": \"🔒\",");
    }
    else if (p->access == PROTECTED) {
        printf("\"access\": \"🔐\",");
    }
    else {
        printf("\"access\": \"🔓\",");
    }
    
    if (returnm == Return) {
        reportType("returnType", p->returnType, tc);
        putc(',', stdout);
    }
    else if (returnm == CanReturnNothingness) {
        printf("\"canReturnNothingness\": true,");
    }
    
    reportGenericArguments(p->genericArgumentVariables, p->genericArgumentConstraints, 0, tc);
    reportDocumentation(p->documentationToken);
    
    printf("\"arguments\": [");
    for (int i = 0; i < p->arguments.size(); i++) {
        printf("{");
        Variable variable = p->arguments[i];
        
        const char *varname = variable.name.value.utf8CString();
        
        reportType("type", variable.type, tc);
        printf(",\"name\":");
        printJSONStringToFile(varname, stdout);
        printf("}%s", i + 1 == p->arguments.size() ? "" : ",");
        
        delete [] varname;
    }
    printf("]");
    
    printf("}%s", last ? "" : ",");
}

void report(Package *package) {
    std::list<Enum *> enums;
    std::list<Class *> classes;
    std::list<Protocol *> protocols;
    
    for (auto exported : package->exportedTypes()) {
        switch (exported.type.type()) {
            case TT_CLASS:
                classes.push_back(exported.type.eclass);
                break;
            case TT_ENUM:
                enums.push_back(exported.type.eenum);
                break;
            case TT_PROTOCOL:
                protocols.push_back(exported.type.protocol);
                break;
            default:
                break;
        }
    }
    
    bool printedClass = false;
    printf("{");
    printf("\"classes\": [");
    for (auto eclass : classes) {
        if (printedClass) {
            putchar(',');
        }
        printedClass = true;
        
        printf("{");
        
        ecCharToCharStack(eclass->name(), className);
        printf("\"name\": \"%s\",", className);

        reportGenericArguments(eclass->ownGenericArgumentVariables(), eclass->genericArgumentConstraints(),
                               eclass->superGenericArguments().size(), TypeContext(eclass));
        reportDocumentation(eclass->documentation());
        
        if (eclass->superclass) {
            ecCharToCharStack(eclass->superclass->name(), superClassName);
            printf("\"superclass\": {\"package\": \"%s\", \"name\": \"%s\"},",
                   eclass->superclass->package()->name(), superClassName);
        }
        
        printf("\"methods\": [");
        for (size_t i = 0; i < eclass->methodList.size(); i++) {
            Method *method = eclass->methodList[i];
            reportProcedureInformation(method, Return, i + 1 == eclass->methodList.size(), TypeContext(eclass, method));
        }
        printf("],");
        
        printf("\"initializers\": [");
        for (size_t i = 0; i < eclass->initializerList.size(); i++) {
            Initializer *initializer = eclass->initializerList[i];
            reportProcedureInformation(initializer, initializer->canReturnNothingness ? CanReturnNothingness : NoReturn,
                                       i + 1 == eclass->initializerList.size(), TypeContext(eclass, initializer));
        }
        printf("],");
        
        printf("\"classMethods\": [");
        for (size_t i = 0; i < eclass->classMethodList.size(); i++) {
            ClassMethod *classMethod = eclass->classMethodList[i];
            reportProcedureInformation(classMethod, Return, eclass->classMethodList.size() == i + 1,
                                       TypeContext(eclass, classMethod));
        }
        printf("],");
        
        printf("\"conformsTo\": [");
        bool printedProtocol = false;
        for (auto protocol : eclass->protocols()) {
            commaPrinter(&printedProtocol);
            reportType(nullptr, protocol, TypeContext(eclass));
        }
        printf("]}");
    }
    printf("],");
    
    printf("\"enums\": [");
    bool printedEnum = false;
    for (auto eenum : enums) {
        if (printedEnum) {
            putchar(',');
        }
        printf("{");
        
        printedEnum = true;
        
        ecCharToCharStack(eenum->name(), enumName);
        printf("\"name\": \"%s\",", enumName);
        
        reportDocumentation(eenum->documentation());
        
        bool printedValue = false;
        printf("\"values\": [");
        for (auto it : eenum->values()) {
            ecCharToCharStack(it.first, value);
            if (printedValue) {
                putchar(',');
            }
            printf("\"%s\"", value);
            printedValue = true;
        }
        printf("]}");
    }
    printf("],");
    
    printf("\"protocols\": [");
    bool printedProtocol = false;
    for (auto protocol : protocols) {
        if (printedProtocol) {
            putchar(',');
        }
        printedProtocol = true;
        printf("{");
        
        ecCharToCharStack(protocol->name(), protocolName);
        printf("\"name\": \"%s\",", protocolName);
        
        reportGenericArguments(protocol->ownGenericArgumentVariables(), protocol->genericArgumentConstraints(),
                               protocol->superGenericArguments().size(), Type(protocol, false));
        reportDocumentation(protocol->documentation());
        
        printf("\"methods\": [");
        for (size_t i = 0; i < protocol->methods().size(); i++) {
            Method *method = protocol->methods()[i];
            reportProcedureInformation(method, Return, i + 1 == protocol->methods().size(),
                                       TypeContext(Type(protocol, false)));
        }
        printf("]}");
    }
    printf("]");
    printf("}");
}