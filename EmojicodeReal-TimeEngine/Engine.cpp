//
//  Emojicode.c
//  Emojicode
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Engine.hpp"
#include "Class.hpp"
#include "Memory.hpp"
#include "Processor.hpp"
#include "Reader.hpp"
#include "Thread.hpp"
#include "ThreadsManager.hpp"
#include <cstdarg>
#include <cstdlib>

using namespace Emojicode;

namespace Emojicode {

#if __SIZEOF_DOUBLE__ != 8
#warning Double does not match the size of an 64-bit integer
#endif

Class *CL_STRING;
Class *CL_LIST;
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
BoxObjectVariableRecords *boxObjectVariableRecordTable;
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

}  // namespace Emojicode

int main(int argc, char *argv[]) {
    cliArgumentCount = argc;
    cliArguments = argv;

    const char *ppath = getenv("EMOJICODE_PACKAGES_PATH");
    if (ppath != nullptr) {
        packageDirectory = ppath;
    }

    if (argc < 2) {
        error("No file provided.");
    }

    FILE *f = fopen(argv[1], "rb");
    if (f == nullptr || ferror(f) > 0) {
        error("File couldn't be opened.");
    }

    Thread *mainThread = ThreadsManager::allocateThread();

    allocateHeap();

    Function *handler = readBytecode(f);
    mainThread->pushStackFrame(Value(), false, handler);
    mainThread->configureInterruption();
    execute(mainThread);
    return 0; // static_cast<int>(mainThread->rstackPop().raw);
}
