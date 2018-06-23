//
//  PackageReporter.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef PackageReporter_hpp
#define PackageReporter_hpp

#define RAPIDJSON_HAS_STDSTRING 1

#include "Types/Generic.hpp"
#include "Utils/rapidjson/prettywriter.h"
#include "Utils/rapidjson/ostreamwrapper.h"

namespace EmojicodeCompiler {

class Package;
class Type;
class TypeContext;
class Function;

namespace CLI {

class PackageReporter {
public:
    explicit PackageReporter(Package *package);

    void report();

private:
    rapidjson::OStreamWrapper wrapper_;
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer_;
    Package *package_;

    void reportDocumentation(const std::u32string &documentation);
    void reportType(const Type &type, const TypeContext &tc);
    void reportFunction(Function *function, const TypeContext &tc);

    template <typename T, typename E>
    void reportGenericParameters(Generic<T, E> *generic, const TypeContext &tc)  {
        writer_.Key("genericParameters");
        writer_.StartArray();
        for (auto &param : generic->genericParameters()) {
            writer_.StartObject();
            writer_.Key("name");
            writer_.String(utf8(param.name));
            writer_.Key("constraint");
            reportType(param.constraint->type(), tc);
            writer_.EndObject();
        }
        writer_.EndArray();
    }

    void reportExportedType(const Type &type);

    template <typename T>
    void printFunctions(const std::vector<T *> &functions, const Type &type)  {
        writer_.StartArray();
        for (auto function : functions) {
            reportFunction(function, TypeContext(type, function));
        }
        writer_.EndArray();
    }

    void reportGenericArguments(const Type &type, const TypeContext &context);

    void reportTypeTypeAndGenericArgs(const char *typeTypeString, const Type &type, const TypeContext &context);
};

}  // namespace CLI

}  // namespace EmojicodeCompiler

#endif /* PackageReporter_hpp */
