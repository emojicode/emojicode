//
//  Emojicode.c
//  Emojicode
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Engine.hpp"
#include <cstdarg>
#include <cstdlib>
#include "Thread.hpp"
#include "Processor.hpp"
#include "Class.hpp"
#include "Object.hpp"
#include "Reader.hpp"

Class *CL_STRING;
Class *CL_LIST;
Class *CL_ERROR;
Class *CL_DATA;
Class *CL_DICTIONARY;
Class *CL_CAPTURED_FUNCTION_CALL;
Class *CL_CLOSURE;

static Class cl_array(nullptr);
Class *CL_ARRAY = &cl_array;

char **cliArguments;
int cliArgumentCount;

const char *packageDirectory = defaultPackagesDirectory;

Class **classTable;
Function **functionTable;
ProtocolDispatchTable *protocolDispatchTableTable;
uint32_t protocolDTTOffset;

uint_fast16_t stringPoolCount;
Object **stringPool;

[[noreturn]] void error(const char *err, ...) {
    va_list list;
    va_start(list, err);

    char error[350];
    vsprintf(error, err, list);

    fprintf(stderr, "🚨 Fatal Error: %s\n", error);

    va_end(list);
    abort();
}

int main(int argc, char *argv[]) {
    cliArgumentCount = argc;
    cliArguments = argv;
    const char *ppath;
    if ((ppath = getenv("EMOJICODE_PACKAGES_PATH"))) {
        packageDirectory = ppath;
    }

    if (argc < 2) {
        error("No file provided.");
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f || ferror(f)) {
        error("File couldn't be opened.");
    }

    Thread mainThread = Thread();

    allocateHeap();

    Function *handler = readBytecode(f);
    Value sth = EmojicodeInteger(0);
    performFunction(handler, Value(), &mainThread, &sth);
    return static_cast<int>(sth.raw);
}
