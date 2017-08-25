//
//  main.c
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 27.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeCompiler.hpp"
#include "Application.hpp"
#include "PackageReporter.hpp"
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

} // namespace EmojicodeCompiler

using EmojicodeCompiler::Application;
using EmojicodeCompiler::outputJSON;
//using EmojicodeCompiler::InformationDesk;

void cliWarning(const std::string &message) {
    puts(message.c_str());
}

int main(int argc, char * argv[]) {
    try {
        const char *packageToReport = nullptr;
        std::string outPath;
        std::string sizeVariable;
        std::string packageDirectory = defaultPackagesDirectory;

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
            cliWarning("No input file provided.");
            return 1;
        }
        if (argc > 1) {
            cliWarning("Only the first file provided will be compiled.");
        }

        if (outPath.empty()) {
            outPath = argv[0];
            outPath[outPath.size() - 1] = 'b';
        }

        auto application = Application(argv[0], std::move(outPath), std::move(packageDirectory));
        bool successfullyCompiled = application.compile();

        if (packageToReport != nullptr) {
            if (auto package = application.findPackage(packageToReport)) {
                reportPackage(package);
            }
            else {
                cliWarning("Report for package %s failed as it was not loaded.");
            }
        }

        if (!sizeVariable.empty()) {
//            InformationDesk(&pkg).sizeOfVariable(sizeVariable);
        }

        if (outputJSON) {
            fprintf(stderr, "]");
        }

        return successfullyCompiled ? 0 : 1;
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
