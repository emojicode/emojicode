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
#include "../utf8.h"
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

    printf("\"documentation\":");
    printJSONStringToFile(documentation.utf8().c_str(), stdout);
    putc(',', stdout);
}

void reportType(const char *key, Type type, TypeContext tc) {
    auto returnTypeName = type.toString(tc, false);

    if (key) {
        printf("\"%s\":{\"package\":\"%s\",\"name\":\"%s\",\"optional\":%s}",
               key, type.typePackage().c_str(), returnTypeName.c_str(), type.optional() ? "true" : "false");
    }
    else {
        printf("{\"package\":\"%s\",\"name\":\"%s\",\"optional\":%s}",
               type.typePackage().c_str(), returnTypeName.c_str(), type.optional() ? "true" : "false");
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
        gans[it.second.reference() - superCount] = it.first;
    }

    auto reported = false;

    for (size_t i = 0; i < gans.size(); i++) {
        auto gan = gans[i];
        commaPrinter(&reported);
        printf("{\"name\":");
        printJSONStringToFile(gan.utf8().c_str(), stdout);
        printf(",");
        reportType("constraint", constraints[i], tc);
        printf("}");
    }

    printf("],");
}

void reportFunctionInformation(Function *p, ReturnManner returnm, bool last, TypeContext tc) {
    printf("{");
    printf("\"name\":\"%s\",", p->name().utf8().c_str());
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

        reportType("type", argument.type, tc);
        printf(",\"name\":");
        printJSONStringToFile(argument.name.value().utf8().c_str(), stdout);
        printf("}%s", i + 1 == p->arguments.size() ? "" : ",");
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
    printJSONStringToFile(package->documentation().utf8().c_str(), stdout);

    bool printedValueType = false;
    printf(",\"valueTypes\":[");
    for (auto vt : valueTypes) {
        auto vttype = Type(vt, false, false, false);
        auto vtcontext = TypeContext(vttype);
        if (printedValueType) {
            putchar(',');
        }
        printedValueType = true;

        printf("{");

        printf("\"name\":\"%s\",", vt->name().utf8().c_str());

        reportGenericArguments(vt->ownGenericArgumentVariables(), vt->genericArgumentConstraints(),
                               vt->superGenericArguments().size(), vtcontext);
        reportDocumentation(vt->documentation());

        printf("\"methods\":[");
        for (size_t i = 0; i < vt->methodList().size(); i++) {
            Function *method = vt->methodList()[i];
            reportFunctionInformation(method, Return, i + 1 == vt->methodList().size(), TypeContext(vttype, method));
        }
        printf("],");

        printf("\"initializers\":[");
        for (size_t i = 0; i < vt->initializerList().size(); i++) {
            Initializer *initializer = vt->initializerList()[i];
            reportFunctionInformation(initializer, initializer->canReturnNothingness ? CanReturnNothingness : NoReturn,
                                      i + 1 == vt->initializerList().size(), TypeContext(vttype, initializer));
        }
        printf("],");

        printf("\"classMethods\":[");
        for (size_t i = 0; i < vt->typeMethodList().size(); i++) {
            Function *classMethod = vt->typeMethodList()[i];
            reportFunctionInformation(classMethod, Return, vt->typeMethodList().size() == i + 1,
                                      TypeContext(vttype, classMethod));
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

        printf("\"name\": \"%s\",", eclass->name().utf8().c_str());

        reportGenericArguments(eclass->ownGenericArgumentVariables(), eclass->genericArgumentConstraints(),
                               eclass->superGenericArguments().size(), TypeContext(Type(eclass, false)));
        reportDocumentation(eclass->documentation());

        if (eclass->superclass()) {
            printf("\"superclass\":{\"package\":\"%s\",\"name\":\"%s\"},",
                   eclass->superclass()->package()->name().c_str(), eclass->superclass()->name().utf8().c_str());
        }

        printf("\"methods\":[");
        for (size_t i = 0; i < eclass->methodList().size(); i++) {
            Function *method = eclass->methodList()[i];
            reportFunctionInformation(method, Return, i + 1 == eclass->methodList().size(),
                                      TypeContext(Type(eclass, false), method));
        }
        printf("],");

        printf("\"initializers\":[");
        for (size_t i = 0; i < eclass->initializerList().size(); i++) {
            Initializer *initializer = eclass->initializerList()[i];
            reportFunctionInformation(initializer, initializer->canReturnNothingness ? CanReturnNothingness : NoReturn,
                                      i + 1 == eclass->initializerList().size(),
                                      TypeContext(Type(eclass, false), initializer));
        }
        printf("],");

        printf("\"classMethods\":[");
        for (size_t i = 0; i < eclass->typeMethodList().size(); i++) {
            Function *classMethod = eclass->typeMethodList()[i];
            reportFunctionInformation(classMethod, Return, eclass->typeMethodList().size() == i + 1,
                                      TypeContext(Type(eclass, false), classMethod));
        }
        printf("],");

        printf("\"conformsTo\":[");
        bool printedProtocol = false;
        for (auto protocol : eclass->protocols()) {
            commaPrinter(&printedProtocol);
            reportType(nullptr, protocol, TypeContext(Type(eclass, false)));
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

        printf("\"name\":\"%s\",", eenum->name().utf8().c_str());

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
            printf("\"value\":\"%s\"}", it.first.utf8().c_str());
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

        printf("\"name\":\"%s\",", protocol->name().utf8().c_str());

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
