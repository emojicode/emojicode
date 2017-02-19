//
//  Reader.c
//  Emojicode
//
//  Created by Theo Weidmann on 01.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Reader.hpp"
#include <dlfcn.h>
#include <cstring>
#include <cstdlib>
#include "Engine.hpp"
#include "Class.hpp"

#ifdef DEBUG
#define DEBUG_LOG(format, ...) printf(format "\n", ##__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif

uint16_t readUInt16(FILE *in) {
    return ((uint16_t)fgetc(in)) | (fgetc(in) << 8);
}

EmojicodeChar readEmojicodeChar(FILE *in) {
    return static_cast<EmojicodeChar>(fgetc(in)) | (fgetc(in) << 8) | (fgetc(in) << 16) | (fgetc(in) << 24);
}

EmojicodeInstruction readInstruction(FILE *in) {
    return static_cast<EmojicodeInstruction>(fgetc(in)) | (fgetc(in) << 8) | (fgetc(in) << 16) | (fgetc(in) << 24);
}

PackageLoadingState packageLoad(const char *name, uint16_t major, uint16_t minor,
                                FunctionFunctionPointer **linkingTable, MarkerPointerForClass *mpfc,
                                SizeForClassFunction *sfch) {
    char *path;
    asprintf(&path, "%s/%s-v%d/%s.%s", packageDirectory, name, major, name, "so");

    void* package = dlopen(path, RTLD_LAZY);

    free(path);

    if (!package)
        return PACKAGE_LOADING_FAILED;

    *linkingTable = static_cast<FunctionFunctionPointer *>(dlsym(package, "linkingTable"));
    *mpfc = (MarkerPointerForClass)(dlsym(package, "markerPointerForClass"));
    *sfch = (SizeForClassFunction)(dlsym(package, "sizeForClass"));

    PackageVersion pv = *static_cast<PackageVersion *>(dlsym(package, "version"));

    if (!*linkingTable)
        return PACKAGE_LOADING_FAILED;

    if (pv.major != major)
        return PACKAGE_INAPPROPRIATE_MAJOR;

    if (pv.minor < minor)
        return PACKAGE_INAPPROPRIATE_MINOR;

    return PACKAGE_LOADED;
}

char* packageError() {
    return dlerror();
}

void readFunction(Function **table, FILE *in, FunctionFunctionPointer *linkingTable) {
    uint16_t vti = readUInt16(in);

    Function *function = static_cast<Function *>(malloc(sizeof(Function)));
    function->argumentCount = fgetc(in);

    DEBUG_LOG("*️⃣ Reading function with vti %d and takes %d argument(s)", vti, function->argumentCount);

    function->objectVariableRecordsCount = readUInt16(in);
    function->objectVariableRecords = new FunctionObjectVariableRecord[function->objectVariableRecordsCount];
    for (int i = 0; i < function->objectVariableRecordsCount; i++) {
        function->objectVariableRecords[i].variableIndex = readUInt16(in);
        function->objectVariableRecords[i].condition = readUInt16(in);
        function->objectVariableRecords[i].type = static_cast<ObjectVariableType>(readUInt16(in));
        function->objectVariableRecords[i].from = readInstruction(in);
        function->objectVariableRecords[i].to = readInstruction(in);
    }

    DEBUG_LOG("Read %d object variable records", function->objectVariableRecordsCount);

    function->frameSize = readUInt16(in);
    uint16_t native = readUInt16(in);
    if (native) {
        DEBUG_LOG("Function is native");
        function->native = true;
        function->handler = linkingTable[native];
    }
    else {
        function->native = false;
        function->block.instructionCount = readEmojicodeChar(in);

        function->block.instructions = new EmojicodeInstruction[function->block.instructionCount];
        for (unsigned int i = 0; i < function->block.instructionCount; i++) {
            function->block.instructions[i] = readInstruction(in);
        }

        DEBUG_LOG("Read block with %d coins and %d local variable(s)", function->block.instructionCount,
                  function->frameSize);
    }
    table[vti] = function;
}

void readProtocolAgreement(Function **vmt, Function ***pmt, uint_fast16_t offset, FILE *in) {
    uint_fast16_t index = readUInt16(in) - offset;
    uint_fast16_t count = readUInt16(in);
    pmt[index] = new Function*[count];
    DEBUG_LOG("Reading protocol %d agreement: %d method(s)", index + offset, count);
    for (uint_fast16_t i = 0; i < count; i++) {
        const uint_fast16_t vti = readUInt16(in);
        DEBUG_LOG("Protocol agreement method VTI %d", vti);
        pmt[index][i] = vmt[vti];
    }
}

void readProtocolTable(ProtocolDispatchTable &table, Function **functionTable, FILE *in) {
    uint_fast16_t protocolCount = readUInt16(in);
    DEBUG_LOG("Reading %d protocol(s)", protocolCount);
    if (protocolCount > 0) {
        table.protocolsMaxIndex = readUInt16(in);
        table.protocolsOffset = readUInt16(in);
        table.protocolsTable = new Function**[table.protocolsMaxIndex - table.protocolsOffset + 1]();

        DEBUG_LOG("Protocol max index is %d, offset is %d", table.protocolsMaxIndex, table.protocolsOffset);

        for (uint_fast16_t i = 0; i < protocolCount; i++) {
            readProtocolAgreement(functionTable, table.protocolsTable, table.protocolsOffset, in);
        }
    }
    else {
        table.protocolsTable = nullptr;
    }
}

void readPackage(FILE *in) {
    static uint16_t classNextIndex = 0;

    FunctionFunctionPointer *linkingTable;
    MarkerPointerForClass mpfc;
    SizeForClassFunction sfch;

    uint_fast8_t packageNameLength = fgetc(in);
    if (!packageNameLength) {
        DEBUG_LOG("Package does not have native binary");
        linkingTable = sLinkingTable;
        mpfc = markerPointerForClass;
        sfch = sizeForClass;
    }
    else {
        DEBUG_LOG("Package has native binary");
        char *name = new char[packageNameLength];
        fread(name, sizeof(char), packageNameLength, in);

        uint16_t major = readUInt16(in);
        uint16_t minor = readUInt16(in);

        DEBUG_LOG("Package is named %s and has version %d.%d.x", name, major, minor);

        PackageLoadingState s = packageLoad(name, major, minor, &linkingTable, &mpfc, &sfch);

        if (s == PACKAGE_INAPPROPRIATE_MAJOR) {
            error("Installed version of package \"%s\" is incompatible with required version %d.%d. (How did you made Emojicode load this version of the package?!)", name, major, minor);
        }
        else if (s == PACKAGE_INAPPROPRIATE_MINOR) {
            error("Installed version of package \"%s\" is incompatible with required version %d.%d. Please update to the latest minor version.", name, major, minor);
        }
        else if (s == PACKAGE_LOADING_FAILED) {
            error("Could not load package \"%s\" %s.", name, packageError());
        }

        delete [] name;
    }

    for (int classCount = readUInt16(in); classCount; classCount--) {
        DEBUG_LOG("➡️ Still %d class(es) to load", classCount);
        EmojicodeChar name = readEmojicodeChar(in);

        Class *klass = new Class;
        classTable[classNextIndex++] = klass;

        DEBUG_LOG("Loading class %X into %p", name, klass);

        klass->superclass = classTable[readUInt16(in)];
        int instanceVariableCount = readUInt16(in);

        int methodCount = readUInt16(in);
        klass->methodsVtable = new Function*[methodCount];

        bool inheritsInitializers = fgetc(in);
        int initializerCount = readUInt16(in);
        klass->initializersVtable = new Function*[initializerCount];

        DEBUG_LOG("Inherting intializers: %s", inheritsInitializers ? "true" : "false");

        DEBUG_LOG("%d instance variable(s); %d methods; %d initializer(s)",
                  instanceVariableCount, initializerCount, methodCount);

        uint_fast16_t localMethodCount = readUInt16(in);
        uint_fast16_t localInitializerCount = readUInt16(in);

        DEBUG_LOG("Reading %d method(s) and %d initializer(s) that are not inherited or overriden",
                  localMethodCount, localInitializerCount);

        if (klass != klass->superclass) {
            memcpy(klass->methodsVtable, klass->superclass->methodsVtable, methodCount * sizeof(Function*));
            if (inheritsInitializers) {
                memcpy(klass->initializersVtable, klass->superclass->initializersVtable,
                       initializerCount * sizeof(Function*));
            }
        }
        else {
            klass->superclass = nullptr;
        }

        for (uint_fast16_t i = 0; i < localMethodCount; i++) {
            DEBUG_LOG("Reading method %d", i);
            readFunction(klass->methodsVtable, in, linkingTable);
        }

        for (uint_fast16_t i = 0; i < localInitializerCount; i++) {
            DEBUG_LOG("Reading initializer %d", i);
            readFunction(klass->initializersVtable, in, linkingTable);
        }

        readProtocolTable(klass->protocolTable, klass->methodsVtable, in);

        klass->mark = mpfc(name);
        size_t size = sfch(klass, name);
        klass->valueSize = klass->superclass && klass->superclass->valueSize ? klass->superclass->valueSize : size;
        klass->size = klass->valueSize + instanceVariableCount * sizeof(Value);

        klass->instanceVariableRecordsCount = readUInt16(in);
        klass->instanceVariableRecords = new FunctionObjectVariableRecord[klass->instanceVariableRecordsCount];
        for (int i = 0; i < klass->instanceVariableRecordsCount; i++) {
            klass->instanceVariableRecords[i].variableIndex = readUInt16(in);
            klass->instanceVariableRecords[i].condition = readUInt16(in);
            klass->instanceVariableRecords[i].type = static_cast<ObjectVariableType>(readUInt16(in));
        }

        DEBUG_LOG("Read %d object variable records", klass->instanceVariableRecordsCount);
    }

    for (int functionCount = readUInt16(in); functionCount; functionCount--) {
        DEBUG_LOG("➡️ Still %d functions to come", functionCount);
        readFunction(functionTable, in, linkingTable);
    }
}

Function* readBytecode(FILE *in) {
    uint8_t version = fgetc(in);
    if (version != BYTE_CODE_VERSION) {
        error("The bytecode file (bcsv %d) is not compatible with this interpreter (bcsv %d).\n",
              version, BYTE_CODE_VERSION);
    }

    DEBUG_LOG("Bytecode version %d", version);

    const int classCount = readUInt16(in);
    classTable = new Class*[classCount];

    DEBUG_LOG("%d class(es) on the whole", classCount);

    const int functionCount = readUInt16(in);
    functionTable = new Function*[functionCount];

    DEBUG_LOG("%d function(s) on the whole", functionCount);

    for (int i = 0, l = fgetc(in); i < l; i++) {
        DEBUG_LOG("Reading package %d of %d", i + 1, l);
        readPackage(in);
    }

    CL_STRING = classTable[0];
    CL_LIST = classTable[1];
    CL_ERROR = classTable[2];
    CL_DATA = classTable[3];
    CL_DICTIONARY = classTable[4];
    CL_CAPTURED_FUNCTION_CALL = classTable[5];
    CL_CLOSURE = classTable[6];

    DEBUG_LOG("✅ Read all packages");

    uint16_t tableSize = readUInt16(in);
    protocolDispatchTableTable = new ProtocolDispatchTable[tableSize];
    protocolDTTOffset = readUInt16(in);
    for (uint16_t count = readUInt16(in); count; count--) {
        DEBUG_LOG("➡️ Still %d value type protocol tables to load", count);
        auto index = readUInt16(in);
        readProtocolTable(protocolDispatchTableTable[index - protocolDTTOffset], functionTable, in);
    }

    stringPoolCount = readUInt16(in);
    DEBUG_LOG("Reading string pool with %d strings", stringPoolCount);
    stringPool = new Object*[stringPoolCount];
    for (int i = 0; i < stringPoolCount; i++) {
        Object *o = newObject(CL_STRING);
        String *string = static_cast<String *>(o->value);

        string->length = readUInt16(in);
        string->characters = newArray(string->length * sizeof(EmojicodeChar));

        for (int j = 0; j < string->length; j++) {
            ((EmojicodeChar*)string->characters->value)[j] = readEmojicodeChar(in);
        }

        stringPool[i] = o;
    }

    DEBUG_LOG("✅ Program ready for execution");
    return functionTable[0];
}
