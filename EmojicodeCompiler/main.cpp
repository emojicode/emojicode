//
//  main.c
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 27.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include <libgen.h>
#include <getopt.h>
#include <cstring>
#include <vector>
#include "Lexer.hpp"
#include "utf8.h"
#include "FileParser.hpp"
#include "StaticAnalyzer.hpp"
#include "Class.hpp"
#include "EmojicodeCompiler.hpp"

StartingFlag startingFlag;
bool foundStartingFlag;

std::vector<Class *> classes;

char* EmojicodeString::utf8CString() const {
    // Size needed for UTF8 representation
    size_t ds = u8_codingsize(c_str(), size());
    char *utf8str = new char[ds + 1];
    size_t written = u8_toutf8(utf8str, ds, c_str(), size());
    utf8str[written] = 0;
    return utf8str;
}

static bool outputJSON = false;
static bool gaveWarning = false;

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

//MARK: Warnings

void compilerError(const Token *token, const char *err, ...) {
    va_list list;
    va_start(list, err);
    
    const char *file;
    char error[450];
    vsprintf(error, err, list);
    
    size_t line, col;
    if (token) {
        line = token->position.line;
        col = token->position.character;
        file = token->position.file;
    }
    else {
        line = col = 0;
        file = "";
    }
    
    if (outputJSON) {
        fprintf(stderr, "%s{\"type\": \"error\", \"line\": %zu, \"character\": %zu, \"file\":", gaveWarning ? ",": "",
                line, col);
        printJSONStringToFile(file, stderr);
        fprintf(stderr, ", \"message\":");
        printJSONStringToFile(error, stderr);
        fprintf(stderr, "}\n]");
    }
    else {
        fprintf(stderr, "🚨 line %zu column %zu %s: %s\n", line, col, file, error);
    }
    
    va_end(list);
    exit(1);
}

void compilerWarning(const Token *token, const char *err, ...) {
    va_list list;
    va_start(list, err);
    
    const char *file;
    char error[450];
    vsprintf(error, err, list);
    
    size_t line, col;
    if (token) {
        line = token->position.line;
        col = token->position.character;
        file = token->position.file;
    }
    else {
        line = col = 0;
        file = "";
    }
    
    if (outputJSON) {
        fprintf(stderr, "%s{\"type\": \"warning\", \"line\": %zu, \"character\": %zu, \"file\":", gaveWarning ? ",": "",
                line, col);
        printJSONStringToFile(file, stderr);
        fprintf(stderr, ", \"message\":");
        printJSONStringToFile(error, stderr);
        fprintf(stderr, "}\n");
    }
    else {
        fprintf(stderr, "⚠️ line %zu col %zu %s: %s\n", line, col, file, error);
    }
    gaveWarning = true;
    
    va_end(list);
}

Class* getStandardClass(EmojicodeChar name, Package *_) {
    Type type = typeNothingness;
    _->fetchRawType(name, globalNamespace, false, nullptr, &type);
    if (type.type() != TT_CLASS) {
        ecCharToCharStack(name, nameString)
        compilerError(nullptr, "s package class %s is missing.", nameString);
    }
    return type.eclass;
}

Protocol* getStandardProtocol(EmojicodeChar name, Package *_) {
    Type type = typeNothingness;
    _->fetchRawType(name, globalNamespace, false, nullptr, &type);
    if (type.type() != TT_PROTOCOL) {
        ecCharToCharStack(name, nameString)
        compilerError(nullptr, "s package protocol %s is missing.", nameString);
    }
    return type.protocol;
}

void loadStandard(Package *_) {
    auto package = _->loadPackage("s", globalNamespace, nullptr);
    
    CL_STRING = getStandardClass(0x1F521, _);
    CL_LIST = getStandardClass(0x1F368, _);
    CL_ERROR = getStandardClass(0x1F6A8, _);
    CL_DATA = getStandardClass(0x1F4C7, _);
    CL_ENUMERATOR = getStandardClass(0x1F361, _);
    CL_DICTIONARY = getStandardClass(0x1F36F, _);
    CL_RANGE = getStandardClass(0x23E9, _);
    
    PR_ENUMERATEABLE = getStandardProtocol(E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY, _);
    
    package->setRequiresBinary(false);
}

int main(int argc, char * argv[]) {
    const char *reportPackage = nullptr;
    char *outPath = nullptr;
    
    signed char ch;
    while ((ch = getopt(argc, argv, "vrjR:o:")) != -1) {
        switch (ch) {
            case 'v':
                puts("Emojicode Compiler 1.0.0alpha1. Emojicode 0.2. Built with 💚 by Theo Weidmann.");
                return 0;
                break;
            case 'R':
                reportPackage = optarg;
                break;
            case 'r':
                reportPackage = "_";
                break;
            case 'o':
                outPath = optarg;
                break;
            case 'j':
                outputJSON = true;
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
        compilerWarning(nullptr, "No input files provided.");
        return 1;
    }
    
    if (outPath == nullptr) {
        outPath = strdup(argv[0]);
        outPath[strlen(outPath) - 1] = 'b';
    }
    
    foundStartingFlag = false;
    
    Package pkg = Package("_");
    pkg.setPackageVersion(PackageVersion(1, 0));
    
    loadStandard(&pkg);
    
    pkg.parse(argv[0], nullptr);
    
    FILE *out = fopen(outPath, "wb");
    if (!out || ferror(out)) {
        compilerError(nullptr, "Couldn't write output file.");
        return 1;
    }
    
    if (!foundStartingFlag) {
        compilerError(nullptr, "No 🏁 eclass method was found.");
    }
    
    analyzeClassesAndWrite(out);
    
    if (outputJSON) {
        fprintf(stderr, "]");
    }
    
    if (reportPackage) {
        if (auto package = Package::findPackage(reportPackage)) {
            report(package);
        }
        else {
            compilerWarning(nullptr, "Report for package %s failed as it was not loaded.", reportPackage);
        }
    }
    
    return 0;
}
