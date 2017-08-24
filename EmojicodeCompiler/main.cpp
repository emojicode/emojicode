//
//  main.c
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 27.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Generation/Writer.hpp"
#include "CompilerError.hpp"
#include "EmojicodeCompiler.hpp"
#include "Function.hpp"
#include "Generation/CodeGenerator.hpp"
#include "PackageReporter.hpp"
#include "Types/Class.hpp"
#include "Types/ValueType.hpp"
#include <codecvt>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <libgen.h>
#include <locale>
#include <unistd.h>
#include <vector>

namespace EmojicodeCompiler {

std::string utf8(const std::u32string &s) {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
    return conv.to_bytes(s);
}

static bool outputJSON = false;
static bool printedErrorOrWarning = false;
static bool hasError = false;
std::string packageDirectory = defaultPackagesDirectory;

void printJSONStringToFile(const char *string, FILE *f) {
    char c;
    fputc('"', f);
    while ((c = *(string++))) {
        switch (c) {
            case '\\':
                fprintf(f, "\\\\");
                break;
            case '"':
                fprintf(f, "\\\"");
                break;
            case '/':
                fprintf(f, "\\/");
                break;
            case '\b':
                fprintf(f, "\\b");
                break;
            case '\f':
                fprintf(f, "\\f");
                break;
            case '\n':
                fprintf(f, "\\n");
                break;
            case '\r':
                fprintf(f, "\\r");
                break;
            case '\t':
                fprintf(f, "\\t");
                break;
            default:
                fputc(c, f);
        }
    }
    fputc('"', f);
}

// MARK: Warnings

void printError(const CompilerError &ce) {
    hasError = true;
    if (outputJSON) {
        fprintf(stderr, "%s{\"type\": \"error\", \"line\": %zu, \"character\": %zu, \"file\":",
                printedErrorOrWarning ? ",": "", ce.position().line, ce.position().character);
        printJSONStringToFile(ce.position().file.c_str(), stderr);
        fprintf(stderr, ", \"message\":");
        printJSONStringToFile(ce.message().c_str(), stderr);
        fprintf(stderr, "}\n");
    }
    else {
        fprintf(stderr, "🚨 line %zu column %zu %s: %s\n", ce.position().line, ce.position().character,
                ce.position().file.c_str(), ce.message().c_str());
    }
    printedErrorOrWarning = true;
}

void compilerWarning(const SourcePosition &p, const std::string &warning) {
    if (outputJSON) {
        fprintf(stderr, "%s{\"type\": \"warning\", \"line\": %zu, \"character\": %zu, \"file\":",
                printedErrorOrWarning ? ",": "", p.line, p.character);
        printJSONStringToFile(p.file.c_str(), stderr);
        fprintf(stderr, ", \"message\":");
        printJSONStringToFile(warning.c_str(), stderr);
        fprintf(stderr, "}\n");
    }
    else {
        fprintf(stderr, "⚠️ line %zu col %zu %s: %s\n", p.line, p.character, p.file.c_str(), warning.c_str());
    }
    printedErrorOrWarning = true;
}

} // namespace EmojicodeCompiler

using EmojicodeCompiler::compilerWarning;
using EmojicodeCompiler::outputJSON;
using EmojicodeCompiler::packageDirectory;
using EmojicodeCompiler::SourcePosition;
using EmojicodeCompiler::Package;
using EmojicodeCompiler::hasError;
using EmojicodeCompiler::Function;
using EmojicodeCompiler::Writer;
using EmojicodeCompiler::PackageVersion;
using EmojicodeCompiler::CompilerError;
//using EmojicodeCompiler::InformationDesk;

int main(int argc, char * argv[]) {
    try {
        const char *packageToReport = nullptr;
        std::string outPath;
        std::string sizeVariable;

        const char *ppath;
        if ((ppath = getenv("EMOJICODE_PACKAGES_PATH")) != nullptr) {
            packageDirectory = ppath;
        }

        signed char ch;
        while ((ch = getopt(argc, argv, "vrjR:o:S:")) != -1) {
            switch (ch) {
                case 'v':
                    puts("Emojicode 0.5. Created by Theo Weidmann.");
                    return 0;
                case 'R':
                    packageToReport = optarg;
                    break;
                case 'r':
                    packageToReport = "_";
                    break;
                case 'o':
                    outPath = optarg;
                    break;
                case 'j':
                    outputJSON = true;
                    break;
                case 'S':
                    sizeVariable = optarg;
                    break;
                default:
                    break;
            }
        }
        argc -= optind;
        argv += optind;

        if (outputJSON) {
            fprintf(stderr, "[");
        }

        if (argc == 0) {
            compilerWarning(SourcePosition(0, 0, ""), "No input file provided.");
            return 1;
        }
        if (argc > 1) {
            compilerWarning(SourcePosition(0, 0, ""), "Only the first file provided will be compiled.");
        }

        if (outPath.empty()) {
            outPath = argv[0];
            outPath[outPath.size() - 1] = 'b';
        }

        Package underscorePackage = Package("_", argv[0]);
        underscorePackage.setPackageVersion(PackageVersion(1, 0));
        underscorePackage.setRequiresBinary(false);

        try {
            underscorePackage.compile();

            if (!Function::foundStart) {
                throw CompilerError(underscorePackage.position(), "No 🏁 block was found.");
            }

            if (!hasError) {
                Writer writer = Writer(outPath);
                generateCode(&writer);
                writer.finish();
            }
        }
        catch (CompilerError &ce) {
            printError(ce);
        }

        if (packageToReport != nullptr) {
            if (auto package = Package::findPackage(packageToReport)) {
                reportPackage(package);
            }
            else {
                compilerWarning(SourcePosition(0, 0, ""), "Report for package %s failed as it was not loaded.",
                                packageToReport);
            }
        }

        if (!sizeVariable.empty()) {
//            InformationDesk(&pkg).sizeOfVariable(sizeVariable);
        }

        if (outputJSON) {
            fprintf(stderr, "]");
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
