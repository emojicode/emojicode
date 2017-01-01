//
//  main.c
//  Packages
//
//  Created by Theo Weidmann on 31.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#define _XOPEN_SOURCE 500
#include "EmojicodeAPI.h"
#include "EmojicodeString.h"
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ftw.h>

PackageVersion getVersion(){
    return (PackageVersion){0, 1};
}

#define handleNEP(failureCondition)\
if((failureCondition)) {\
    *destination = somethingObject(newError(strerror(errno), errno));\
return;\
}\
else\
    *destination = NOTHINGNESS;


void filesMkdir(Thread *thread, Something *destination) {
    int state = mkdir(stringToChar(stackGetVariable(0, thread).object->value), 0755);

    handleNEP(state != 0);
}

void filesSymlink(Thread *thread, Something *destination) {
    char *s = stringToChar(stackGetVariable(0, thread).object->value);
    int state = symlink(s, stringToChar(stackGetVariable(1, thread).object->value));
    free(s);

    handleNEP(state != 0);
}

void filesFileExists(Thread *thread, Something *destination) {
    char *s = stringToChar(stackGetVariable(0, thread).object->value);
    *destination = (access(s, F_OK) == 0) ? EMOJICODE_TRUE : EMOJICODE_FALSE;
    free(s);
}

void filesIsReadable(Thread *thread, Something *destination) {
    char *s = stringToChar(stackGetVariable(0, thread).object->value);
    *destination = (access(s, R_OK) == 0) ? EMOJICODE_TRUE : EMOJICODE_FALSE;
    free(s);
}

void filesIsWriteable(Thread *thread, Something *destination) {
    char *s = stringToChar(stackGetVariable(0, thread).object->value);
    *destination = (access(s, W_OK) == 0)  ? EMOJICODE_TRUE : EMOJICODE_FALSE;
    free(s);
}

void filesIsExecuteable(Thread *thread, Something *destination) {
    char *s = stringToChar(stackGetVariable(0, thread).object->value);
    *destination = (access(s, X_OK) == 0)  ? EMOJICODE_TRUE : EMOJICODE_FALSE;
    free(s);
}

void filesRemove(Thread *thread, Something *destination) {
    char *s = stringToChar(stackGetVariable(0, thread).object->value);
    int state = remove(s);
    free(s);

    handleNEP(state != 0);
}

void filesRmdir(Thread *thread, Something *destination) {
    char *s = stringToChar(stackGetVariable(0, thread).object->value);
    int state = rmdir(s);
    free(s);

    handleNEP(state != 0);
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

void filesRecursiveRmdir(Thread *thread, Something *destination) {
    char *s = stringToChar(stackGetVariable(0, thread).object->value);

    int state = nftw(s, filesRecursiveRmdirHelper, 64, FTW_DEPTH | FTW_PHYS);
    handleNEP(state != 0);

    free(s);
}

void filesSize(Thread *thread, Something *destination) {
    char *s = stringToChar(stackGetVariable(0, thread).object->value);

    FILE *file = fopen(s, "r");
    free(s);

    if (file == NULL) {
        *destination = somethingInteger(-1);
        return;
    }

    fseek(file, 0L, SEEK_END);

    long length = ftell(file);

    fclose(file);

    *destination = somethingInteger((EmojicodeInteger)length);
}

void filesRealpath(Thread *thread, Something *destination) {
    char path[PATH_MAX];
    char *s = stringToChar(stackGetVariable(0, thread).object->value);
    char *x = realpath(s, path);

    free(s);

    if (!x) {
        *destination = NOTHINGNESS;
    }
    else {
        *destination = somethingObject(stringFromChar(path));
    }
}

//MARK: file

//Shortcuts

void fileDataPut(Thread *thread, Something *destination) {
    char *s = stringToChar(stackGetVariable(0, thread).object->value);
    FILE *file = fopen(s, "wb");
    free(s);

    handleNEP(file == NULL);

    Data *d = stackGetVariable(1, thread).object->value;

    fwrite(d->bytes, 1, d->length, file);

    handleNEP(ferror(file));

    fclose(file);
}

void fileDataGet(Thread *thread, Something *destination) {
    char *s = stringToChar(stackGetVariable(0, thread).object->value);
    FILE *file = fopen(s, "rb");
    free(s);

    if(file == NULL){
        *destination = NOTHINGNESS;
        return;
    }

    int state = fseek(file, 0, SEEK_END);

    long length = ftell(file);

    state = fseek(file, 0, SEEK_SET);

    Object *bytesObject = newArray(length);
    fread(bytesObject->value, 1, length, file);
    if(ferror(file)){
        fclose(file);
        *destination = NOTHINGNESS;
        return;
    }
    fclose(file);

    stackPush(somethingObject(bytesObject), 0, 0, thread);

    Object *obj = newObject(CL_DATA);
    Data *data = obj->value;
    data->length = length;
    data->bytesObject = stackGetThisObject(thread);
    data->bytes = data->bytesObject->value;

    stackPop(thread);

    *destination = somethingObject(obj);
}

#define file(obj) (*((FILE**)(obj)->value))

void fileStdinGet(Thread *thread, Something *destination) {
    Object *obj = newObject(stackGetThisContext(thread).eclass);
    file(obj) = stdin;
    *destination = somethingObject(obj);
}

void fileStdoutGet(Thread *thread, Something *destination) {
    Object *obj = newObject(stackGetThisContext(thread).eclass);
    file(obj) = stdout;
    *destination = somethingObject(obj);
}

void fileStderrGet(Thread *thread, Something *destination) {
    Object *obj = newObject(stackGetThisContext(thread).eclass);
    file(obj) = stderr;
    *destination = somethingObject(obj);
}

//Constructors

void fileForWriting(Thread *thread, Something *destination) {
    char *p = stringToChar(stackGetVariable(0, thread).object->value);
    FILE *f = fopen(p, "wb");
    if (f){
        file(stackGetThisObject(thread)) = f;
    }
    else {
        *destination = NOTHINGNESS;
    }
    free(p);
}

void fileForReading(Thread *thread, Something *destination) {
    char *p = stringToChar(stackGetVariable(0, thread).object->value);
    FILE *f = fopen(p, "rb");
    if (f){
        file(stackGetThisObject(thread)) = f;
    }
    else {
        *destination = NOTHINGNESS;
    }
    free(p);
}

void fileWriteData(Thread *thread, Something *destination) {
    FILE *f = file(stackGetThisObject(thread));
    Data *d = stackGetVariable(0, thread).object->value;

    fwrite(d->bytes, 1, d->length, f);
    fflush(f);

    handleNEP(ferror(f));
}

void fileReadData(Thread *thread, Something *destination) {
    FILE *f = file(stackGetThisObject(thread));
    EmojicodeInteger n = unwrapInteger(stackGetVariable(0, thread));

    Object *bytesObject = newArray(n);

    size_t read = fread(bytesObject->value, 1, n, f);

    if (read != n || ferror(f)) {
        *destination = NOTHINGNESS;
        return;
    }

    stackPush(somethingObject(bytesObject), 0, 0, thread);

    Object *obj = newObject(CL_DATA);
    Data *data = obj->value;
    data->length = n;
    data->bytesObject = stackGetThisObject(thread);
    data->bytes = data->bytesObject->value;

    stackPop(thread);

    *destination = somethingObject(obj);
}

void fileSeekTo(Thread *thread, Something *destination) {
    fseek(file(stackGetThisObject(thread)), unwrapInteger(stackGetVariable(0, thread)), SEEK_SET);
}

void fileSeekToEnd(Thread *thread, Something *destination) {
    fseek(file(stackGetThisObject(thread)), 0, SEEK_END);
}

void closeFile(void *file){
    fclose((*((FILE**)file)));
}

LinkingTable {
    [1] = filesMkdir,
    [2] = filesSymlink,
    [3] = filesFileExists,
    [4] = filesIsReadable,
    [5] = filesIsWriteable,
    [6] = filesIsExecuteable,
    [7] = filesRemove,
    [8] = filesRmdir,
    [9] = filesRecursiveRmdir,
    [10] = filesSize,
    [11] = filesRealpath,
    [12] = fileDataPut,
    [13] = fileDataGet,
    [14] = fileStdinGet,
    [15] = fileStdoutGet,
    [16] = fileStderrGet,
    [17] = fileWriteData,
    [18] = fileReadData,
    [19] = fileSeekTo,
    [20] = fileSeekToEnd,
    [22] = fileForWriting,
    [23] = fileForReading
};

Marker markerPointerForClass(EmojicodeChar cl){
    return NULL;
}

uint_fast32_t sizeForClass(Class *cl, EmojicodeChar name) {
    switch (name) {
        case 0x1F4C4:
            return sizeof(FILE*);
    }
    return 0;
}

Deinitializer deinitializerPointerForClass(EmojicodeChar cl){
    if(cl == 0x1F4C4){
        return closeFile;
    }
    return NULL;
}
