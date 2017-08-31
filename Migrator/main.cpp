//
//  main.c
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 27.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "utf8.h"
#include "Class.hpp"
#include "CodeGenerator.hpp"
#include "CompilerError.hpp"
#include "EmojicodeCompiler.hpp"
#include "Function.hpp"
#include "ValueType.hpp"
#include "MigWriter.hpp"
#include <cstdlib>
#include <cstring>
#include <vector>
#include <iostream>

namespace EmojicodeCompiler {

std::string EmojicodeString::utf8() const {
    std::string string;
    string.resize(u8_codingsize(c_str(), size()));
    u8_toutf8(&string[0], string.capacity(), c_str(), size());
    return string;
}

static bool hasError = false;
std::string packageDirectory = defaultPackagesDirectory;

// MARK: Warnings

void printError(const CompilerError &ce) {
    hasError = true;
    fprintf(stderr, "🚨 line %zu column %zu %s: %s\n", ce.position().line, ce.position().character,
            ce.position().file.c_str(), ce.error());
}

void compilerWarning(const SourcePosition &p, const char *err, ...) {
    va_list list;
    va_start(list, err);

    char error[450];
    vsprintf(error, err, list);

    fprintf(stderr, "⚠️ line %zu col %zu %s: %s\n", p.line, p.character, p.file.c_str(), error);
    va_end(list);
}

Class* getStandardClass(const EmojicodeString &name, Package *_, const SourcePosition &errorPosition) {
    Type type = Type::nothingness();
    _->fetchRawType(name, globalNamespace, false, errorPosition, &type);
    if (type.type() != TypeContent::Class) {
        throw CompilerError(errorPosition, "s package class %s is missing.", name.utf8().c_str());
    }
    return type.eclass();
}

Protocol* getStandardProtocol(const EmojicodeString &name, Package *_, const SourcePosition &errorPosition) {
    Type type = Type::nothingness();
    _->fetchRawType(name, globalNamespace, false, errorPosition, &type);
    if (type.type() != TypeContent::Protocol) {
        throw CompilerError(errorPosition, "s package protocol %s is missing.", name.utf8().c_str());
    }
    return type.protocol();
}

ValueType* getStandardValueType(const EmojicodeString &name, Package *_, const SourcePosition &errorPosition,
                                unsigned int boxId) {
    Type type = Type::nothingness();
    _->fetchRawType(name, globalNamespace, false, errorPosition, &type);
    if (type.type() != TypeContent::ValueType) {
        throw CompilerError(errorPosition, "s package value type %s is missing.", name.utf8().c_str());
    }
    if (type.boxIdentifier() != boxId) {
        throw CompilerError(errorPosition, "s package value type %s has improper box id.", name.utf8().c_str());
    }
    return type.valueType();
}

void loadStandard(Package *_, const SourcePosition &errorPosition) {
    auto package = _->loadPackage("s", globalNamespace, errorPosition);

    // Order of the following calls is important as they will cause Box IDs to be assigned
    VT_BOOLEAN = getStandardValueType(EmojicodeString(E_OK_HAND_SIGN), _, errorPosition, T_BOOLEAN);
    VT_INTEGER = getStandardValueType(EmojicodeString(E_STEAM_LOCOMOTIVE), _, errorPosition, T_INTEGER);
    VT_DOUBLE = getStandardValueType(EmojicodeString(E_ROCKET), _, errorPosition, T_DOUBLE);
    VT_SYMBOL = getStandardValueType(EmojicodeString(E_INPUT_SYMBOL_FOR_SYMBOLS), _, errorPosition, T_SYMBOL);

    CL_STRING = getStandardClass(EmojicodeString(0x1F521), _, errorPosition);
    CL_LIST = getStandardClass(EmojicodeString(0x1F368), _, errorPosition);
    CL_DATA = getStandardClass(EmojicodeString(0x1F4C7), _, errorPosition);
    CL_DICTIONARY = getStandardClass(EmojicodeString(0x1F36F), _, errorPosition);

    PR_ENUMERATOR = getStandardProtocol(EmojicodeString(0x1F361), _, errorPosition);
    PR_ENUMERATEABLE = getStandardProtocol(EmojicodeString(E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY), _, errorPosition);

    package->setRequiresBinary(false);
}

}

using EmojicodeCompiler::compilerWarning;
using EmojicodeCompiler::packageDirectory;
using EmojicodeCompiler::SourcePosition;
using EmojicodeCompiler::Package;
using EmojicodeCompiler::hasError;
using EmojicodeCompiler::Function;
using EmojicodeCompiler::MigWriter;
using EmojicodeCompiler::PackageVersion;
using EmojicodeCompiler::CompilerError;
using EmojicodeCompiler::generateCode;

int main(int argc, char * argv[]) {
    try {
        const char *ppath;
        if ((ppath = getenv("EMOJICODE_PACKAGES_PATH")) != nullptr) {
            packageDirectory = ppath;
        }

        if (argc == 1) {
            compilerWarning(SourcePosition(0, 0, ""), "No input file provided.");
            return 1;
        }
        if (argc > 2) {
            compilerWarning(SourcePosition(0, 0, ""), "Only the first file provided will be compiled.");
        }

        std::string outPath = argv[1];
        outPath.replace(outPath.size() - 6, 6, "emojimig");

        std::cout << "👩‍🏭 Emojicode Source Code Migrator 0.5 ➡️ 0.6\n";

        auto errorPosition = SourcePosition(0, 0, argv[1]);

        Package pkg = Package("_", errorPosition);
        pkg.setPackageVersion(PackageVersion(1, 0));

        try {
            loadStandard(&pkg, errorPosition);

            pkg.parse(argv[1]);

            if (!Function::foundStart) {
                throw CompilerError(errorPosition, "No 🏁 block was found.");
            }

            auto writer = MigWriter(outPath, &pkg);
            generateCode(&writer);
            std::cout << "📝 Wrote migration file to " << outPath << "\n";
            std::cout << "👩‍💻 Letting compiler create new files" << "\n";
            if (std::system(("./emojicodec " + outPath).c_str()) == 0) {
                std::cout << "✅ Done" << "\n";
            }
        }
        catch (CompilerError &ce) {
            printError(ce);
        }

        if (hasError) {
            return 1;
        }
    }
    catch (std::exception &ex) {
        printf("💣 The compiler crashed due to an internal problem: %s\nPlease report this message and the code that "
               "you were trying to compile as an issue on GitHub.", ex.what());
        return 1;
    }
    catch (...) {
        printf("💣 The compiler crashed due to an unidentifiable internal problem.\nPlease report this message and the "
               "code that you were trying to compile as an issue on GitHub.");
        return 1;
    }

    return 0;
}
