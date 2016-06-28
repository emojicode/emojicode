//
//  Reader.c
//  Emojicode
//
//  Created by Theo Weidmann on 01.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Emojicode.h"
#include <string.h>
#include <dlfcn.h>

uint16_t readUInt16(FILE *in){
    return ((uint16_t)fgetc(in)) | (fgetc(in) << 8);
}

EmojicodeChar readEmojicodeChar(FILE *in){
    return ((EmojicodeChar)fgetc(in)) | (fgetc(in) << 8) | ((EmojicodeChar)fgetc(in) << 16) | ((EmojicodeChar)fgetc(in) << 24);
}

EmojicodeCoin readCoin(FILE *in){
    return ((EmojicodeCoin)fgetc(in)) | (fgetc(in) << 8) | ((EmojicodeCoin)fgetc(in) << 16) | ((EmojicodeCoin)fgetc(in) << 24);
}

PackageLoadingState packageLoad(const char *name, uint16_t major, uint16_t minor, FunctionFunctionPointerProvider *hfpMethods,
                                InitializerFunctionFunctionPointerProvider *hfpIntializer,
                                mpfc *mpfc, dpfc *dpfc, SizeForClassFunction *sfch){
    char *path;
    asprintf(&path, "%s/%s-v%d/%s.%s", packageDirectory, name, major, name, "so");
    
    void* package = dlopen(path, RTLD_LAZY);
    
    free(path);
    
    if(!package)
        return PACKAGE_LOADING_FAILED;
    
    *hfpMethods = dlsym(package, "handlerPointerForMethod");
    *hfpIntializer = dlsym(package, "handlerPointerForInitializer");
    *mpfc = dlsym(package, "markerPointerForClass");
    *dpfc = dlsym(package, "deinitializerPointerForClass");
    *sfch = dlsym(package, "sizeForClass");
    
    PackageVersion(*pvf)() = dlsym(package, "getVersion");
    PackageVersion pv = pvf();
    
    if(!*hfpMethods || !*hfpIntializer)
        return PACKAGE_LOADING_FAILED;
    
    if(pv.major != major)
        return PACKAGE_INAPPROPRIATE_MAJOR;

    if(pv.minor < minor)
        return PACKAGE_INAPPROPRIATE_MINOR;
    
    return PACKAGE_LOADED;
}

char* packageError(){
    return dlerror();
}

void readQualifiedTypeName(EmojicodeChar *name, EmojicodeChar *namespace, FILE *in){
    *name = readEmojicodeChar(in);
    *namespace = readEmojicodeChar(in);
}

uint32_t readBlock(EmojicodeCoin **destination, uint8_t *variableCount, FILE *in){
    *variableCount = fgetc(in);
    uint32_t coinCount = readEmojicodeChar(in);

    *destination = malloc(sizeof(EmojicodeCoin) * coinCount);
    for (uint32_t i = 0; i < coinCount; i++) {
        (*destination)[i] = readCoin(in);
    }
    
    return coinCount;
}

void readInitializer(InitializerFunction **table, EmojicodeChar className, FILE *in, InitializerFunctionFunctionPointerProvider hpfc){
    EmojicodeChar name = readEmojicodeChar(in);
    uint16_t vti = readUInt16(in);

    InitializerFunction *initializer = malloc(sizeof(InitializerFunction));
    initializer->argumentCount = fgetc(in);
    
    if (fgetc(in)) {
        initializer->native = true;
        initializer->handler = hpfc(className, name);
    }
    else {
        initializer->native = false;
        initializer->tokenCount = readBlock(&initializer->tokenStream, &initializer->variableCount, in);
    }
    table[vti] = initializer;
}

void readFunction(Function **table, EmojicodeChar className, FILE *in, FunctionFunctionPointerProvider hpfm){
    EmojicodeChar methodName = readEmojicodeChar(in);
    uint16_t vti = readUInt16(in);
    
    Function *method = malloc(sizeof(Function));
    method->argumentCount = fgetc(in);
    
    MethodType nativeType;
    if ((nativeType = fgetc(in))) {
        method->native = true;
        method->handler = hpfm(className, methodName, nativeType);
    }
    else {
        method->native = false;
        method->tokenCount = readBlock(&method->tokenStream, &method->variableCount, in);
    }
    table[vti] = method;
}

void readProtocolAgreement(Function **vmt, Function ***pmt, uint_fast16_t offset, FILE *in){
    uint_fast16_t index = readUInt16(in) - offset;
    uint_fast16_t count = readUInt16(in);
    pmt[index] = malloc(sizeof(Function *) * count);
    for (uint_fast16_t i = 0; i < count; i++) {
        pmt[index][i] = vmt[readUInt16(in)];
    }
}

void readPackage(FILE *in){
    static uint16_t classNextIndex = 0;
    
    FunctionFunctionPointerProvider hfpMethods;
    InitializerFunctionFunctionPointerProvider hfpIntializer;
    dpfc dpfc;
    mpfc mpfc;
    SizeForClassFunction sfch;
    
    uint_fast8_t packageNameLength = fgetc(in);
    if (!packageNameLength) {
        hfpMethods = handlerPointerForMethod;
        hfpIntializer = handlerPointerForInitializer;
        dpfc = deinitializerPointerForClass;
        mpfc = markerPointerForClass;
        sfch = sizeForClass;
    }
    else {
        char *name = malloc(sizeof(char) * packageNameLength);
        fread(name, sizeof(char), packageNameLength, in);
        
        uint16_t major = readUInt16(in);
        uint16_t minor = readUInt16(in);
        
        PackageLoadingState s = packageLoad(name, major, minor, &hfpMethods, &hfpIntializer,
                                            &mpfc, &dpfc, &sfch);
        
        if (s == PACKAGE_INAPPROPRIATE_MAJOR) {
            error("Installed version of package \"%s\" is incompatible with required version %d.%d. (How did you made Emojicode load this version of the package?!)", name, major, minor);
        }
        else if (s == PACKAGE_INAPPROPRIATE_MINOR) {
            error("Installed version of package \"%s\" is incompatible with required version %d.%d. Please update to the latest minor version.", name, major, minor);
        }
        else if (s == PACKAGE_LOADING_FAILED) {
            error("Could not load package \"%s\" %s.", name, packageError());
        }
        
        free(name);
    }
    
    for (uint_fast16_t classCount = readUInt16(in); classCount; classCount--) {
        EmojicodeChar name = readEmojicodeChar(in);
        
        Class *class = malloc(sizeof(Class));
        classTable[classNextIndex++] = class;
        
        class->superclass = classTable[readUInt16(in)];
        class->instanceVariableCount = readUInt16(in);
        
        class->methodCount = readUInt16(in);
        class->methodsVtable = malloc(sizeof(Function*) * class->methodCount);
        
        bool inheritsInitializers = fgetc(in);
        class->initializerCount = readUInt16(in);
        class->initializersVtable = malloc(sizeof(InitializerFunction*) * class->initializerCount);
        
        uint_fast16_t localMethodCount = readUInt16(in);
        uint_fast16_t localInitializerCount = readUInt16(in);
        
        if (class != class->superclass) {
            memcpy(class->methodsVtable, class->superclass->methodsVtable, class->superclass->methodCount * sizeof(Function*));
            if (inheritsInitializers) {
                memcpy(class->initializersVtable, class->superclass->initializersVtable, class->superclass->initializerCount * sizeof(InitializerFunction*));
            }
        }
        else {
            class->superclass = NULL;
        }
        
        for (uint_fast16_t i = 0; i < localMethodCount; i++) {
            readFunction(class->methodsVtable, name, in, hfpMethods);
        }
        
        for (uint_fast16_t i = 0; i < localInitializerCount; i++) {
            readInitializer(class->initializersVtable, name, in, hfpIntializer);
        }
        
        uint_fast16_t protocolCount = readUInt16(in);
        if(protocolCount > 0){
            class->protocolsMaxIndex = readUInt16(in);
            class->protocolsOffset = readUInt16(in);
            class->protocolsTable = calloc((class->protocolsMaxIndex - class->protocolsOffset + 1), sizeof(Function **));
            
            for (uint_fast16_t i = 0; i < protocolCount; i++) {
                readProtocolAgreement(class->methodsVtable, class->protocolsTable, class->protocolsOffset, in);
            }
        }
        else {
            class->protocolsTable = NULL;
        }
        
        class->deconstruct = dpfc(name);
        if(!class->deconstruct && class->superclass){
            class->deconstruct = class->superclass->deconstruct;
        }
        class->mark = mpfc(name);
        class->valueSize = class->superclass && class->superclass->valueSize ? class->superclass->valueSize : sfch(class, name);
        class->size = class->valueSize + class->instanceVariableCount * sizeof(Something);
    }
}

Function* readBytecode(FILE *in) {
    uint8_t version = fgetc(in);
    if (version != ByteCodeSpecificationVersion) {
        error("The bytecode file (bcsv %d) is not compatible with this interpreter (bcsv %d).\n", version, ByteCodeSpecificationVersion);
    }
    
    classTable = malloc(sizeof(Class*) * readUInt16(in));
    
    for (uint8_t i = 0, l = fgetc(in); i < l; i++) {
        readPackage(in);
    }
    
    CL_STRING = classTable[0];
    CL_LIST = classTable[1];
    CL_ERROR = classTable[2];
    CL_DATA = classTable[3];
    CL_DICTIONARY = classTable[4];
    CL_CAPTURED_FUNCTION_CALL = classTable[5];
    CL_CLOSURE = classTable[6];
    CL_RANGE = classTable[7];
    
    functionTable = malloc(sizeof(Function*) * readUInt16(in));
    for (uint_fast16_t functionSectionCount = readUInt16(in); functionSectionCount; functionSectionCount--) {
        EmojicodeChar name = readEmojicodeChar(in);
        for (uint_fast16_t functionCount = readUInt16(in); functionCount; functionCount--) {
            readFunction(functionTable, name, in, handlerPointerForMethod);
        }
    }
    
    stringPoolCount = readUInt16(in);
    stringPool = malloc(sizeof(Object*) * stringPoolCount);
    for (uint16_t i = 0; i < stringPoolCount; i++) {
        Object *o = newObject(CL_STRING);
        String *string = o->value;

        string->length = readUInt16(in);
        string->characters = newArray(string->length * sizeof(EmojicodeChar));
        
        for (uint16_t j = 0; j < string->length; j++) {
            ((EmojicodeChar*)string->characters->value)[j] = readEmojicodeChar(in);
        }

        stringPool[i] = o;
    }
    
    return functionTable[0];
}
