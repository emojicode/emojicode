//
//  PackageReporter.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include <cstring>
#include <vector>
#include <list>
#include <map>
#include "utf8.h"
#include "Function.hpp"
#include "Class.hpp"
#include "EmojicodeCompiler.hpp"
#include "Enum.hpp"
#include "Protocol.hpp"
#include "PackageReporter.hpp"
#include "TypeContext.hpp"
#include "ValueType.hpp"

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
    auto returnTypeName = type.toString(tc, false);
    
    if (key) {
        printf("\"%s\":{\"package\":\"%s\",\"name\":\"%s\",\"optional\":%s}",
               key, type.typePackage(), returnTypeName.c_str(), type.optional() ? "true" : "false");
    }
    else {
        printf("{\"package\":\"%s\",\"name\":\"%s\",\"optional\":%s}",
               type.typePackage(), returnTypeName.c_str(), type.optional() ? "true" : "false");
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
    printf("\"genericArguments\":[");
    
    auto gans = std::vector<EmojicodeString>(map.size());
    for (auto it : map) {
        gans[it.second.reference - superCount] = it.first;
    }
    
    auto reported = false;
    
    for (size_t i = 0; i < gans.size(); i++) {
        auto gan = gans[i];
        auto utf8 = gan.utf8CString();
        commaPrinter(&reported);
        printf("{\"name\":");
        printJSONStringToFile(utf8, stdout);
        printf(",");
        reportType("constraint", constraints[i], tc);
        printf("}");
        delete [] utf8;
    }
    
    printf("],");
}

void reportFunctionInformation(Function *p, ReturnManner returnm, bool last, TypeContext tc) {
    ecCharToCharStack(p->name(), nameString);
    
    printf("{");
    printf("\"name\":\"%s\",", nameString);
    if (p->accessLevel() == AccessLevel::Private) {
        printf("\"access\":\"🔒\",");
    }
    else if (p->accessLevel() == AccessLevel::Protected) {
        printf("\"access\":\"🔐\",");
    }
    else {
        printf("\"access\":\"🔓\",");
    }
    
    if (returnm == Return) {
        reportType("returnType", p->returnType, tc);
        putc(',', stdout);
    }
    else if (returnm == CanReturnNothingness) {
        printf("\"canReturnNothingness\":true,");
    }
    
    reportGenericArguments(p->genericArgumentVariables, p->genericArgumentConstraints, 0, tc);
    reportDocumentation(p->documentation());
    
    printf("\"arguments\":[");
    for (int i = 0; i < p->arguments.size(); i++) {
        printf("{");
        auto argument = p->arguments[i];
        
        const char *varname = argument.name.value.utf8CString();
        
        reportType("type", argument.type, tc);
        printf(",\"name\":");
        printJSONStringToFile(varname, stdout);
        printf("}%s", i + 1 == p->arguments.size() ? "" : ",");
        
        delete [] varname;
    }
    printf("]");
    
    printf("}%s", last ? "" : ",");
}

void reportPackage(Package *package) {
    std::list<Enum *> enums;
    std::list<Class *> classes;
    std::list<Protocol *> protocols;
    std::list<ValueType *> valueTypes;
    
    for (auto exported : package->exportedTypes()) {
        switch (exported.type.type()) {
            case TypeContent::Class:
                classes.push_back(exported.type.eclass());
                break;
            case TypeContent::Enum:
                enums.push_back(exported.type.eenum());
                break;
            case TypeContent::Protocol:
                protocols.push_back(exported.type.protocol());
                break;
            case TypeContent::ValueType:
                valueTypes.push_back(exported.type.valueType());
                break;
            default:
                break;
        }
    }
    
    printf("{");
    
    printf("\"documentation\":");
    printJSONStringToFile(package->documentation().utf8CString(), stdout);
    
    bool printedValueType = false;
    printf(",\"valueTypes\":[");
    for (auto vt : valueTypes) {
        auto vtcontext = TypeContext(Type(vt, false));
        if (printedValueType) {
            putchar(',');
        }
        printedValueType = true;
        
        printf("{");
        
        ecCharToCharStack(vt->name(), className);
        printf("\"name\":\"%s\",", className);
        
        reportGenericArguments(vt->ownGenericArgumentVariables(), vt->genericArgumentConstraints(),
                               vt->superGenericArguments().size(), vtcontext);
        reportDocumentation(vt->documentation());
        
        printf("\"methods\":[");
        for (size_t i = 0; i < vt->methodList().size(); i++) {
            Function *method = vt->methodList()[i];
            reportFunctionInformation(method, Return, i + 1 == vt->methodList().size(),
                                      TypeContext(Type(vt, false), method));
        }
        printf("],");
        
        printf("\"initializers\":[");
        for (size_t i = 0; i < vt->initializerList().size(); i++) {
            Initializer *initializer = vt->initializerList()[i];
            reportFunctionInformation(initializer, initializer->canReturnNothingness ? CanReturnNothingness : NoReturn,
                                      i + 1 == vt->initializerList().size(),
                                      TypeContext(Type(vt, false), initializer));
        }
        printf("],");
        
        printf("\"classMethods\":[");
        for (size_t i = 0; i < vt->classMethodList().size(); i++) {
            Function *classMethod = vt->classMethodList()[i];
            reportFunctionInformation(classMethod, Return, vt->classMethodList().size() == i + 1,
                                      TypeContext(Type(vt, false), classMethod));
        }
        printf("]}");
    }
    printf("],");
    
    bool printedClass = false;
    printf("\"classes\":[");
    for (auto eclass : classes) {
        if (printedClass) {
            putchar(',');
        }
        printedClass = true;
        
        printf("{");
        
        ecCharToCharStack(eclass->name(), className);
        printf("\"name\": \"%s\",", className);
        
        reportGenericArguments(eclass->ownGenericArgumentVariables(), eclass->genericArgumentConstraints(),
                               eclass->superGenericArguments().size(), TypeContext(Type(eclass)));
        reportDocumentation(eclass->documentation());
        
        if (eclass->superclass()) {
            ecCharToCharStack(eclass->superclass()->name(), superClassName);
            printf("\"superclass\":{\"package\":\"%s\",\"name\":\"%s\"},",
                   eclass->superclass()->package()->name(), superClassName);
        }
        
        printf("\"methods\":[");
        for (size_t i = 0; i < eclass->methodList().size(); i++) {
            Function *method = eclass->methodList()[i];
            reportFunctionInformation(method, Return, i + 1 == eclass->methodList().size(),
                                       TypeContext(Type(eclass), method));
        }
        printf("],");
        
        printf("\"initializers\":[");
        for (size_t i = 0; i < eclass->initializerList().size(); i++) {
            Initializer *initializer = eclass->initializerList()[i];
            reportFunctionInformation(initializer, initializer->canReturnNothingness ? CanReturnNothingness : NoReturn,
                                       i + 1 == eclass->initializerList().size(),
                                       TypeContext(Type(eclass), initializer));
        }
        printf("],");
        
        printf("\"classMethods\":[");
        for (size_t i = 0; i < eclass->classMethodList().size(); i++) {
            Function *classMethod = eclass->classMethodList()[i];
            reportFunctionInformation(classMethod, Return, eclass->classMethodList().size() == i + 1,
                                       TypeContext(Type(eclass), classMethod));
        }
        printf("],");
        
        printf("\"conformsTo\":[");
        bool printedProtocol = false;
        for (auto protocol : eclass->protocols()) {
            commaPrinter(&printedProtocol);
            reportType(nullptr, protocol, TypeContext(Type(eclass)));
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
        printf("\"name\":\"%s\",", enumName);
        
        reportDocumentation(eenum->documentation());
        
        bool printedValue = false;
        printf("\"values\":[");
        for (auto it : eenum->values()) {
            if (printedValue) {
                putchar(',');
            }
            printedValue = true;
            printf("{");
            reportDocumentation(it.second.second);
            ecCharToCharStack(it.first, value);
            printf("\"value\":\"%s\"}", value);
        }
        printf("]}");
    }
    printf("],");
    
    printf("\"protocols\":[");
    bool printedProtocol = false;
    for (auto protocol : protocols) {
        if (printedProtocol) {
            putchar(',');
        }
        printedProtocol = true;
        printf("{");
        
        ecCharToCharStack(protocol->name(), protocolName);
        printf("\"name\":\"%s\",", protocolName);
        
        reportGenericArguments(protocol->ownGenericArgumentVariables(), protocol->genericArgumentConstraints(),
                               protocol->superGenericArguments().size(), Type(protocol, false));
        reportDocumentation(protocol->documentation());
        
        printf("\"methods\":[");
        for (size_t i = 0; i < protocol->methods().size(); i++) {
            Function *method = protocol->methods()[i];
            reportFunctionInformation(method, Return, i + 1 == protocol->methods().size(),
                                       TypeContext(Type(protocol, false)));
        }
        printf("]}");
    }
    printf("]");
    printf("}");
}
