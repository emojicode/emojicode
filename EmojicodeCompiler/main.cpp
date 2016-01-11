//
//  main.c
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 27.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//
#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "Lexer.hpp"
#include "utf8.h"
#include "EmojicodeCompiler.hpp"
#include "FileParser.hpp"
#include "StaticAnalyzer.hpp"
#include "Class.hpp"

StartingFlag startingFlag;
bool foundStartingFlag;

std::map<std::array<EmojicodeChar, 2>, Class*> classesRegister;
std::map<std::array<EmojicodeChar, 2>, Protocol*> protocolsRegister;
std::map<std::array<EmojicodeChar, 2>, Enum*> enumsRegister;

std::vector<Class *> classes;
std::vector<Package *> packages;


char* EmojicodeString::utf8CString() const {
    //Size needed for UTF8 representation
    size_t ds = u8_codingsize(c_str(), size());
    //Allocate space for the UTF8 string
    char *utf8str = new char[ds + 1];
    //Convert
    size_t written = u8_toutf8(utf8str, ds, c_str(), size());
    utf8str[written] = 0;
    return utf8str;
}

static bool outputJSON = false;
static bool gaveWarning = false;

void printJSONStringToFile(const char *string, FILE *f){
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

void compilerError(const Token *token, const char *err, ...){
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
        fprintf(stderr, "%s{\"type\": \"error\", \"line\": %lu, \"character\": %lu, \"file\":", gaveWarning ? ",": "", line, col);
        printJSONStringToFile(file, stderr);
        fprintf(stderr, ", \"message\":");
        printJSONStringToFile(error, stderr);
        fprintf(stderr, "}\n]");
    }
    else {
        fprintf(stderr, "🚨 line %lu column %lu %s: %s\n", line, col, file, error);
    }
    
    va_end(list);
    exit(1);
}

void compilerWarning(const Token *token, const char *err, ...){
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
        fprintf(stderr, "%s{\"type\": \"warning\", \"line\": %lu, \"character\": %lu, \"file\":", gaveWarning ? ",": "", line, col);
        printJSONStringToFile(file, stderr);
        fprintf(stderr, ", \"message\":");
        printJSONStringToFile(error, stderr);
        fprintf(stderr, "}\n");
    }
    else {
        fprintf(stderr, "⚠️ line %lu col %lu %s: %s\n", line, col, file, error);
    }
    gaveWarning = true;
    
    va_end(list);
}

void loadStandard(){
    packageRegisterHeaderNewest("s", globalNamespace);
    
    CL_STRING = classes[0];
    CL_LIST = classes[1];
    CL_ERROR = classes[2];
    CL_DATA = classes[3];
    CL_ENUMERATOR = classes[4];
    CL_DICTIONARY = classes[5];
    PR_ENUMERATEABLE = getProtocol(E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY, globalNamespace);
}

int main(int argc, char * argv[]) {
    const char *reportPackage = nullptr;
    std::string outPath;
    
    //Parse options
    char ch;
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
    
    if (outputJSON){
        fprintf(stderr, "[");
    }
    
    if(argc == 0){
        compilerWarning(nullptr, "No input files provided.");
        return 1;
    }
    
    if(outPath.size() == 0){
        outPath = std::string(argv[0]);
        outPath[outPath.size() - 1] = 'b';
    }
    
    foundStartingFlag = false;
    
    loadStandard();
    
    Package *pkg = new Package("_", (PackageVersion){1, 0}, false);
    
    packages.push_back(pkg);
    for (int i = 0; i < argc; i++) {
        parseFile(argv[i], pkg, false, globalNamespace);
    }
    
    FILE *out = fopen(outPath.c_str(), "wb");
    if(!out || ferror(out)){
        compilerError(nullptr, "Couldn't write output file.");
        return 1;
    }
    
    //If we did not find a mouse method the programm has no start point
    if (!foundStartingFlag) {
        compilerError(nullptr, "No 🏁 eclass method was found.");
    }
    
    //Now let us anaylze these classes and write them
    analyzeClassesAndWrite(out);
    
    if (outputJSON){
        fprintf(stderr, "]");
    }
    
    //Print a report on request
    if(reportPackage){
        report(reportPackage);
    }
    
    return 0;
}
