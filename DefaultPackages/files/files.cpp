//
//  main.c
//  Packages
//
//  Created by Theo Weidmann on 31.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "../../EmojicodeReal-TimeEngine/EmojicodeAPI.hpp"
#include "../../EmojicodeReal-TimeEngine/Class.hpp"
#include "../../EmojicodeReal-TimeEngine/Data.hpp"
#include "../../EmojicodeReal-TimeEngine/String.hpp"
#include "../../EmojicodeReal-TimeEngine/Thread.hpp"
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ftw.h>
#include <sys/stat.h>
#include <unistd.h>

using Emojicode::Thread;
using Emojicode::Data;
using Emojicode::stringToCString;

Emojicode::EmojicodeInteger errnoToError() {
    switch (errno) {
        case EACCES:
            return 1;
        case EEXIST:
            return 2;
        case ENOMEM:
            return 3;
        case ENOSYS:
            return 4;
        case EDOM:
            return 5;
        case EINVAL:
            return 6;
        case EILSEQ:
            return 7;
        case ERANGE:
            return 8;
        case EPERM:
            return 9;
    }
    return 0;
}

bool nothingnessOrErrorEnum(bool success, Thread *thread) {
    if (success) {
        thread->returnNothingnessFromFunction();
        return false;
    }
    
        thread->returnErrorFromFunction(errnoToError());
        return true;
    
}

void filesMkdir(Thread *thread) {
    int state = mkdir(stringToCString(thread->variable(0).object), 0755);
    nothingnessOrErrorEnum(state == 0, thread);
}

void filesSymlink(Thread *thread) {
    int state = symlink(stringToCString(thread->variable(0).object), stringToCString(thread->variable(1).object));
    nothingnessOrErrorEnum(state == 0, thread);
}

void filesFileExists(Thread *thread) {
    thread->returnFromFunction(access(stringToCString(thread->variable(0).object), F_OK) == 0);
}

void filesIsReadable(Thread *thread) {
    thread->returnFromFunction(access(stringToCString(thread->variable(0).object), R_OK) == 0);
}

void filesIsWriteable(Thread *thread) {
    thread->returnFromFunction(access(stringToCString(thread->variable(0).object), W_OK) == 0);
}

void filesIsExecuteable(Thread *thread) {
    thread->returnFromFunction(access(stringToCString(thread->variable(0).object), X_OK) == 0);
}

void filesRemove(Thread *thread) {
    int state = remove(stringToCString(thread->variable(0).object));
    nothingnessOrErrorEnum(state == 0, thread);
}

void filesRmdir(Thread *thread) {
    int state = rmdir(stringToCString(thread->variable(0).object));
    nothingnessOrErrorEnum(state == 0, thread);
}

int filesRecursiveRmdirHelper(const char *fpath, const struct stat * /*sb*/, int typeflag, struct FTW * /*ftwbuf*/){
    int state;

    if (typeflag == FTW_F || typeflag == FTW_SL || typeflag == FTW_SLN) {
        state = remove(fpath);
    }
    else if (typeflag == FTW_DP) {
        state = rmdir(fpath);
    }
    else {
        state = -4;
    }

    return state;
}

void filesRecursiveRmdir(Thread *thread) {
    int state = nftw(stringToCString(thread->variable(0).object), filesRecursiveRmdirHelper,
                     64, FTW_DEPTH | FTW_PHYS);
    nothingnessOrErrorEnum(state == 0, thread);
}

void filesSize(Thread *thread) {
    struct stat st{};
    int state = stat(stringToCString(thread->variable(0).object), &st);

    if (state == 0) {
        thread->returnOEValueFromFunction(st.st_size);
    }
    else {
        thread->returnErrorFromFunction(errnoToError());
    }
}

void filesRealpath(Thread *thread) {
    char path[PATH_MAX];
    char *x = realpath(stringToCString(thread->variable(0).object), path);

    if (x != nullptr) {
        thread->returnOEValueFromFunction(Emojicode::stringFromChar(path));
    }
    else {
        thread->returnErrorFromFunction(errnoToError());
    }
}

//MARK: file

//Shortcuts

void fileDataPut(Thread *thread) {
    FILE *file = fopen(stringToCString(thread->variable(0).object), "wb");

    if (nothingnessOrErrorEnum(file != nullptr, thread)) {
        return;
    }

    auto *d = thread->variable(1).object->val<Data>();

    fwrite(d->bytes, 1, d->length, file);

    nothingnessOrErrorEnum(ferror(file) == 0, thread);
    fclose(file);
}

void fileDataGet(Thread *thread) {
    FILE *file = fopen(stringToCString(thread->variable(0).object), "rb");

    if (file == nullptr) {
        thread->returnErrorFromFunction(errnoToError());
        return;
    }

    int state = fseek(file, 0, SEEK_END);
    long length = ftell(file);
    state = fseek(file, 0, SEEK_SET);

    auto bytesObject = thread->retain(Emojicode::newArray(length));
    fread(bytesObject->val<char>(), 1, length, file);
    if (ferror(file) != 0) {
        fclose(file);
        thread->release(1);
        thread->returnNothingnessFromFunction();
        return;
    }
    fclose(file);

    Emojicode::Object *obj = Emojicode::newObject(Emojicode::CL_DATA);
    auto *data = obj->val<Data>();
    data->length = length;
    data->bytesObject = bytesObject.unretainedPointer();
    data->bytes = bytesObject->val<char>();

    thread->release(1);
    thread->returnOEValueFromFunction(obj);
}

#define file(obj) (*(obj)->val<FILE*>())

void fileStdinGet(Thread *thread) {
    Emojicode::Object *obj = newObject(thread->thisContext().klass);
    file(obj) = stdin;
    thread->returnFromFunction(obj);
}

void fileStdoutGet(Thread *thread) {
    Emojicode::Object *obj = newObject(thread->thisContext().klass);
    file(obj) = stdout;
    thread->returnFromFunction(obj);
}

void fileStderrGet(Thread *thread) {
    Emojicode::Object *obj = newObject(thread->thisContext().klass);
    file(obj) = stderr;
    thread->returnFromFunction(obj);
}

// Constructors

void fileOpenWithMode(Thread *thread, const char *mode) {
    FILE *f = fopen(stringToCString(thread->variable(0).object), mode);
    if (f != nullptr) {
        file(thread->thisObject()) = f;
        thread->returnOEValueFromFunction(thread->thisContext());
    }
    else {
        thread->returnErrorFromFunction(errnoToError());
    }
}

void fileForWriting(Thread *thread) {
    fileOpenWithMode(thread, "wb");
}

void fileForReading(Thread *thread) {
    fileOpenWithMode(thread, "rb");
}

void fileWriteData(Thread *thread) {
    FILE *f = file(thread->thisObject());
    auto *d = thread->variable(0).object->val<Data>();

    fwrite(d->bytes, 1, d->length, f);
    nothingnessOrErrorEnum(ferror(f) == 0, thread);
}

void fileReadData(Thread *thread) {
    FILE *f = file(thread->thisObject());
    Emojicode::EmojicodeInteger n = thread->variable(0).raw;

    auto bytesObject = thread->retain(Emojicode::newArray(n));
    fread(bytesObject->val<char>(), 1, n, f);
    if (ferror(f) != 0) {
        thread->returnErrorFromFunction(errnoToError());
        thread->release(1);
        return;
    }

    Emojicode::Object *obj = Emojicode::newObject(Emojicode::CL_DATA);
    auto *data = obj->val<Data>();
    data->length = n;
    data->bytesObject = bytesObject.unretainedPointer();
    data->bytes = bytesObject->val<char>();

    thread->returnOEValueFromFunction(obj);
    thread->release(1);
}

void fileSeekTo(Thread *thread) {
    fseek(file(thread->thisObject()), thread->variable(0).raw, SEEK_SET);
    thread->returnFromFunction();
}

void fileSeekToEnd(Thread *thread) {
    fseek(file(thread->thisObject()), 0, SEEK_END);
    thread->returnFromFunction();
}

void fileClose(Thread *thread) {
    fclose(file(thread->thisObject()));
    thread->returnFromFunction();
}

void fileFlush(Thread *thread) {
    fflush(file(thread->thisObject()));
    thread->returnFromFunction();
}

Emojicode::PackageVersion version(0, 1);

LinkingTable {
    nullptr,
    filesMkdir,
    filesSymlink,
    filesFileExists,
    filesIsReadable,
    filesIsWriteable,
    filesIsExecuteable,
    filesRemove,
    filesRmdir,
    filesRecursiveRmdir,
    filesSize,
    filesRealpath,
    fileDataPut,
    fileDataGet,
    fileStdinGet,
    fileStdoutGet,
    fileStderrGet,
    fileWriteData,
    fileReadData,
    fileSeekTo,
    fileSeekToEnd,
    fileForWriting,
    fileForReading,
    fileClose,
    fileFlush,
};

extern "C" void prepareClass(Emojicode::Class *klass, EmojicodeChar name) {
    switch (name) {
        case 0x1F4C4:
            klass->valueSize = sizeof(FILE*);
            break;
    }
}
