//
//  main.c
//  Packages
//
//  Created by Theo Weidmann on 31.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "../../EmojicodeReal-TimeEngine/EmojicodeAPI.hpp"
#include "../../EmojicodeReal-TimeEngine/Thread.hpp"
#include "../../EmojicodeReal-TimeEngine/String.h"
#include "../../EmojicodeReal-TimeEngine/standard.h"
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <climits>
#include <ftw.h>

using Emojicode::Thread;
using Emojicode::Value;
using Emojicode::String;
using Emojicode::Data;
using Emojicode::stringToCString;

Emojicode::EmojicodeInteger errnoToError() {
    switch (errno) {
        case EACCES:
            return 0;
        case EEXIST:
            return 1;
    }
    return 0;
}

bool nothingnessOrErrorEnum(bool success, Value *destination) {
    if (success) {
        destination->makeNothingness();
        return false;
    }
    else {
        destination->optionalSet(errnoToError());
        return true;
    }
}

void filesMkdir(Thread *thread, Value *destination) {
    int state = mkdir(stringToCString(thread->getVariable(0).object), 0755);
    nothingnessOrErrorEnum(state == 0, destination);
}

void filesSymlink(Thread *thread, Value *destination) {
    int state = symlink(stringToCString(thread->getVariable(0).object), stringToCString(thread->getVariable(1).object));
    nothingnessOrErrorEnum(state == 0, destination);
}

void filesFileExists(Thread *thread, Value *destination) {
    destination->raw = access(stringToCString(thread->getVariable(0).object), F_OK) == 0;
}

void filesIsReadable(Thread *thread, Value *destination) {
    destination->raw = access(stringToCString(thread->getVariable(0).object), R_OK) == 0;
}

void filesIsWriteable(Thread *thread, Value *destination) {
    destination->raw = access(stringToCString(thread->getVariable(0).object), W_OK) == 0;
}

void filesIsExecuteable(Thread *thread, Value *destination) {
    destination->raw = access(stringToCString(thread->getVariable(0).object), X_OK) == 0;
}

void filesRemove(Thread *thread, Value *destination) {
    int state = remove(stringToCString(thread->getVariable(0).object));
    nothingnessOrErrorEnum(state == 0, destination);
}

void filesRmdir(Thread *thread, Value *destination) {
    int state = rmdir(stringToCString(thread->getVariable(0).object));
    nothingnessOrErrorEnum(state == 0, destination);
}

int filesRecursiveRmdirHelper(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf){
    int state;

    if(typeflag == FTW_F || typeflag == FTW_SL || typeflag == FTW_SLN){
        state = remove(fpath);
    }
    else if(typeflag == FTW_DP){
        state = rmdir(fpath);
    }
    else {
        state = -4;
    }

    return state;
}

void filesRecursiveRmdir(Thread *thread, Value *destination) {
    int state = nftw(stringToCString(thread->getVariable(0).object), filesRecursiveRmdirHelper,
                     64, FTW_DEPTH | FTW_PHYS);
    nothingnessOrErrorEnum(state == 0, destination);
}

void filesSize(Thread *thread, Value *destination) {
    struct stat st;
    int state = stat(stringToCString(thread->getVariable(0).object), &st);

    if (state == 0) {
        destination->setValueForError(st.st_size);
    }
    else {
        destination->storeError(errnoToError());
    }
}

void filesRealpath(Thread *thread, Value *destination) {
    char path[PATH_MAX];
    char *x = realpath(stringToCString(thread->getVariable(0).object), path);

    if (x != nullptr) {
        destination->setValueForError(Emojicode::stringFromChar(path));
    }
    else {
        destination->storeError(errnoToError());
    }
}

//MARK: file

//Shortcuts

void fileDataPut(Thread *thread, Value *destination) {
    FILE *file = fopen(stringToCString(thread->getVariable(0).object), "wb");

    if (nothingnessOrErrorEnum(file != NULL, destination)) {
        return;
    }

    Data *d = static_cast<Data *>(thread->getVariable(1).object->value);

    fwrite(d->bytes, 1, d->length, file);

    nothingnessOrErrorEnum(ferror(file) == 0, destination);
    fclose(file);
}

void fileDataGet(Thread *thread, Value *destination) {
    FILE *file = fopen(stringToCString(thread->getVariable(0).object), "rb");

    if (file == NULL) {
        destination->storeError(errnoToError());
        return;
    }

    int state = fseek(file, 0, SEEK_END);
    long length = ftell(file);
    state = fseek(file, 0, SEEK_SET);

    Emojicode::Object *const &bytesObject = thread->retain(Emojicode::newArray(length));
    fread(bytesObject->value, 1, length, file);
    if (ferror(file)) {
        fclose(file);
        thread->release(1);
        destination->makeNothingness();
        return;
    }
    fclose(file);

    Emojicode::Object *obj = Emojicode::newObject(Emojicode::CL_DATA);
    Data *data = static_cast<Data *>(obj->value);
    data->length = length;
    data->bytesObject = bytesObject;
    data->bytes = static_cast<char *>(bytesObject->value);

    thread->release(1);
    destination->setValueForError(obj);
}

#define file(obj) (*((FILE**)(obj)->value))

void fileStdinGet(Thread *thread, Value *destination) {
    Emojicode::Object *obj = newObject(thread->getThisContext().klass);
    file(obj) = stdin;
    destination->object = obj;
}

void fileStdoutGet(Thread *thread, Value *destination) {
    Emojicode::Object *obj = newObject(thread->getThisContext().klass);
    file(obj) = stdout;
    destination->object = obj;
}

void fileStderrGet(Thread *thread, Value *destination) {
    Emojicode::Object *obj = newObject(thread->getThisContext().klass);
    file(obj) = stderr;
    destination->object = obj;
}

// Constructors

void fileOpenWithMode(Thread *thread, Value *destination, const char *mode) {
    FILE *f = fopen(stringToCString(thread->getVariable(0).object), mode);
    if (f) {
        file(thread->getThisObject()) = f;
        destination->optionalSet(thread->getThisContext());
    }
    else {
        destination->makeNothingness();
    }
}

void fileForWriting(Thread *thread, Value *destination) {
    fileOpenWithMode(thread, destination, "wb");
}

void fileForReading(Thread *thread, Value *destination) {
    fileOpenWithMode(thread, destination, "rb");
}

void fileWriteData(Thread *thread, Value *destination) {
    FILE *f = file(thread->getThisObject());
    Data *d = static_cast<Data *>(thread->getVariable(0).object->value);

    fwrite(d->bytes, 1, d->length, f);
    nothingnessOrErrorEnum(ferror(f) == 0, destination);
}

void fileReadData(Thread *thread, Value *destination) {
    FILE *f = file(thread->getThisObject());
    Emojicode::EmojicodeInteger n = thread->getVariable(0).raw;

    Emojicode::Object *const &bytesObject = thread->retain(Emojicode::newArray(n));
    fread(bytesObject->value, 1, n, f);
    if (ferror(f) != 0) {
        destination->storeError(errnoToError());
        thread->release(1);
        return;
    }

    Emojicode::Object *obj = Emojicode::newObject(Emojicode::CL_DATA);
    Data *data = static_cast<Data *>(obj->value);
    data->length = n;
    data->bytesObject = bytesObject;
    data->bytes = static_cast<char *>(bytesObject->value);

    destination->setValueForError(obj);
    thread->release(1);
}

void fileSeekTo(Thread *thread, Value *destination) {
    fseek(file(thread->getThisObject()), thread->getVariable(0).raw, SEEK_SET);
}

void fileSeekToEnd(Thread *thread, Value *destination) {
    fseek(file(thread->getThisObject()), 0, SEEK_END);
}

void fileClose(Thread *thread, Value *destination) {
    fclose(file(thread->getThisObject()));
}

void fileFlush(Thread *thread, Value *destination) {
    fflush(file(thread->getThisObject()));
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

extern "C" Emojicode::Marker markerPointerForClass(EmojicodeChar cl) {
    return NULL;
}

extern "C" uint_fast32_t sizeForClass(Emojicode::Class *cl, EmojicodeChar name) {
    switch (name) {
        case 0x1F4C4:
            return sizeof(FILE*);
    }
    return 0;
}
