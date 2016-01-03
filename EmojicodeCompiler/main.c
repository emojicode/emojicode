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

#include "utf8.h"

#include "EmojicodeCompiler.h"
#include "ClassParser.h"
#include "StaticAnalyzer.h"

Token* currentToken;
Token* consumeToken(){
    currentToken = currentToken->nextToken;
    return currentToken;
}

static bool outputJSON = false;
static bool gaveWarning = false;

char* stringToChar(String *str){
    //Size needed for UTF8 representation
    size_t ds = u8_codingsize(str->characters, str->length);
    //Allocate space for the UTF8 string
    char *utf8str = malloc(ds + 1);
    //Convert
    size_t written = u8_toutf8(utf8str, ds, str->characters, str->length);
    utf8str[written] = 0;
    return utf8str;
}

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

void compilerError(Token *token, const char *err, ...){
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
        fprintf(stderr, "🚨 line %lu col %lu %s: %s\n", line, col, file, error);
    }
    
    va_end(list);
    exit(1);
}

void compilerWarning(Token *token, const char *err, ...){
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

const char* tokenTypeToString(TokenType type){
    switch (type) {
        case BOOLEAN_FALSE:
            return "Boolean False";
        case BOOLEAN_TRUE:
            return "Boolean True";
        case DOUBLE:
            return "Float";
        case INTEGER:
            return "Integer";
        case STRING:
            return "String";
        case SYMBOL:
            return "Symbol";
        case VARIABLE:
            return "Variable";
        case IDENTIFIER:
            return "Identifier";
        case DOCUMENTATION_COMMENT:
            return "Documentation Comment";
        default:
            return "Mysterious unnamed token";
            break;
    }
}

/**
 * When @c token is not of type @c type and compiler error is thrown.
 */
void tokenTypeCheck(TokenType type, Token *token){
    if(token == NULL || token->type == NO_TYPE){
        compilerError(token, "Expected token but found end of programm.");
        return;
    }
    if (type != NO_TYPE && token->type != type){
        compilerError(token, "Expected token %s but instead found %s.", tokenTypeToString(type), tokenTypeToString(token->type));
    }
}

void loadStandard(){
    packageRegisterHeaderNewest("s", globalNamespace);
    
    CL_STRING = getList(classes, 0);
    CL_LIST = getList(classes, 1);
    CL_ERROR = getList(classes, 2);
    CL_DATA = getList(classes, 3);
    CL_ENUMERATOR = getList(classes, 4);
    CL_DICTIONARY = getList(classes, 5);
    PR_ENUMERATEABLE = getProtocol(E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY, globalNamespace);
}

int main(int argc, char * argv[]) {
    char *reportPackage = NULL;
    char *outPath = NULL;
    
    //Parse options
    char ch;
    while ((ch = getopt(argc, argv, "vrjR:o:")) != EOF) {
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
        compilerWarning(NULL, "No input files provided.");
        return 1;
    }
    if(outPath == NULL){
        size_t pathLength = strlen(argv[0]);
        outPath = malloc(pathLength + 1);
        strcpy(outPath, argv[0]);
        outPath[pathLength - 1] = 'b';
    }
    
    foundStartingFlag = false;
    
    //Make a global list to store all classes
    classes = newList();
    packages = newList();
    
    initTypes();
    loadStandard();
    
    Package *pkg = malloc(sizeof(Package));
    pkg->version = (PackageVersion){1, 0};
    pkg->name = "_";
    pkg->requiresNativeBinary = false;
    
    appendList(packages, pkg);
    for (int i = 0; i < argc; i++) {
        parseFile(argv[i], pkg, false, globalNamespace);
    }
    
    FILE *out = fopen(outPath, "wb");
    if(!out || ferror(out)){
        compilerError(NULL, "Couldn't write output file.");
        return 1;
    }
    
    //If we did not find a mouse method the programm has no start point
    if (!foundStartingFlag) {
        compilerError(NULL, "No 🏁 class method was found.");
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
