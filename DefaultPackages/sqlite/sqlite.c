//
//  sqlite.c
//  DefaultPackages
//
//  Created by Theo Weidmann on 20/04/15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeAPI.h"
#include "EmojicodeString.h"
#include "EmojicodeDictionary.h"
#include "EmojicodeList.h"
#include "sqlite3.h"
#include <string.h>

#define goSqlite3(obj) (*((sqlite3**)(obj)->value))
#define goSqlite3_stmt(obj) (*((sqlite3_stmt**)(obj)->value))

PackageVersion getVersion(){
    return (PackageVersion){0, 1};
}

static void bridgeSQLiteOpen(Thread *thread){
    char *path = stringToChar(stackGetVariable(0, thread).object->value);
    int state = sqlite3_open(path, (sqlite3**)stackGetThis(thread)->value);
    free(path);
    
    if(state != SQLITE_OK){
        stackGetThis(thread)->value = NULL;
        puts(sqlite3_errmsg(goSqlite3(stackGetThis(thread))));
    }
}

static void bridgeSQLiteRelease(void *db){
    sqlite3_close_v2(db);
}

static void bridgeSQLitePrepare(Thread *thread){
    char *sql = stringToChar(stackGetVariable(1, thread).object->value);
    sqlite3_stmt *statement;
    if(sqlite3_prepare_v2(goSqlite3(stackGetVariable(0, thread).object), sql, -1, &statement, NULL) != SQLITE_OK){
        puts(sqlite3_errmsg(goSqlite3(stackGetVariable(0, thread).object)));
    }
    goSqlite3_stmt(stackGetThis(thread)) = statement;
    free(sql);
}

static Something bridgeSQLiteBindInteger(Thread *thread){
    EmojicodeInteger i = unwrapInteger(stackGetVariable(0, thread));
    Something toBind = stackGetVariable(1, thread);
    
    int state;
    if(toBind.type == T_INTEGER){
        state = sqlite3_bind_int(goSqlite3_stmt(stackGetThis(thread)), (int)i, (int)unwrapInteger(toBind));
    }
    else if(toBind.type == T_OBJECT && instanceof(toBind.object, CL_STRING)){
            String *string = toBind.object->value;
            char *text = stringToChar(string);
            state = sqlite3_bind_text(goSqlite3_stmt(stackGetThis(thread)), (int)i, text, (int)string->length, free);
    }
    else if(toBind.type == T_OBJECT && instanceof(toBind.object, CL_DATA)){
        Data *data = toBind.object->value;
        char* bytes = malloc(data->length);
        memcpy(bytes, data->bytes, data->length);
        state = sqlite3_bind_blob(goSqlite3_stmt(stackGetThis(thread)), (int)i, bytes, (int)data->length, free);
    }
//    else if(toBind.type == T_FLOAT){
//        state = sqlite3_bind_double(stackGetThis(thread)->value, i, *(float *)toBind->value);
//    }
    else if(isNothingness(toBind)){
        state = sqlite3_bind_null(goSqlite3_stmt(stackGetThis(thread)), (int)i);
    }
    else {
        return somethingObject(newError("Unbindable type", -2));
    }
    
    if (state != SQLITE_OK) {
        return somethingObject(newError("See error code", state));
    }
    return NOTHINGNESS;
}

static Something bridgeSQLiteLastError(Thread *thread){
    int code = sqlite3_errcode(stackGetThis(thread)->value);
    if (code != SQLITE_OK) {
        return somethingObject(newError(sqlite3_errmsg(stackGetThis(thread)->value), code));
    }
    return NOTHINGNESS;
}

static Something bridgeSQLiteLastInsertID(Thread *thread){
    return somethingInteger((EmojicodeInteger)sqlite3_last_insert_rowid(stackGetThis(thread)->value));
}

static Something bridgeSQLiteStep(Thread *thread){
    stackPush(NULL, 3, 0, thread);
    
    int colnamed = 0;
    stackSetVariable(0, somethingObject(newObject(CL_LIST)), thread);
    stackSetVariable(1, somethingObject(newObject(CL_LIST)), thread);
    
    while(sqlite3_step(goSqlite3_stmt(stackGetThis(thread))) == SQLITE_ROW){
        Object *dicto = newObject(CL_DICTIONARY);
        stackPush(dicto, 0, 0, thread);
        dictionaryInit(thread);
        dicto = stackGetThis(thread);
        stackPop(thread);
        stackSetVariable(2, somethingObject(dicto), thread);
        
        int columns = sqlite3_column_count(goSqlite3_stmt(stackGetThis(thread)));
        for (int i = 0; i < columns; i++) {
            if(colnamed == i){
                colnamed++;
                
                const char *colname = sqlite3_column_name(goSqlite3_stmt(stackGetThis(thread)), i);
            
                if(colname == NULL){
                    stackPop(thread);
                    return NOTHINGNESS;
                }
            
                listAppend(stackGetVariable(1, thread).object->value, somethingObject(stringFromChar(colname)), thread);
            }
                
            Something sth = NOTHINGNESS;
            switch(sqlite3_column_type(goSqlite3_stmt(stackGetThis(thread)), i)){
                case SQLITE_INTEGER: {
                    sth = somethingInteger(sqlite3_column_int(goSqlite3_stmt(stackGetThis(thread)), i));
                    break;
                }
                case SQLITE_TEXT: {
                    const char *text = (const char*)sqlite3_column_text(goSqlite3_stmt(stackGetThis(thread)), i);
                    if(text != NULL){
                        sth = somethingObject(stringFromChar(text));
                    }
                    break;
                }
                case SQLITE_BLOB: {
                    const char *data = sqlite3_column_blob(goSqlite3_stmt(stackGetThis(thread)), i);
                    if(data != NULL){
                        int length = sqlite3_column_bytes(goSqlite3_stmt(stackGetThis(thread)), i);
                        
                        Object *datao = newObject(CL_DATA);
                        Data *data = datao->value;
                        
                        char* bytes = malloc(length);
                        memcpy(bytes, data, length);
                        
                        data->bytes = bytes;
                        data->length = length;
                        
                        sth = somethingObject(datao);
                    }
                    break;
                }
                case SQLITE_FLOAT: {
                    sth = somethingInteger((EmojicodeInteger)(int)sqlite3_column_double(goSqlite3_stmt(stackGetThis(thread)), i));
                    break;
                }
                // SQLITE_NULL: nothing to do
            }
            
            dictionarySet(stackGetVariable(2, thread).object->value, listGet(stackGetVariable(1, thread).object->value, i).object, sth, thread);
        }
        
        listAppend(stackGetVariable(0, thread).object->value, somethingObject(dicto), thread);
    }
    Something sth = stackGetVariable(0, thread);
    stackPop(thread);
    return sth;
}

static void bridgeSQLiteFinalize(void *stmt){
    sqlite3_finalize(stmt);
}

ClassMethodHandler handlerPointerForClassMethod(EmojicodeChar cl, EmojicodeChar symbol){
    return NULL;
}

MethodHandler handlerPointerForMethod(EmojicodeChar cl, EmojicodeChar symbol){
    switch (cl) {
        case 0x1F4DA:
            switch (symbol) {
                case 0x1F6A8:
                    return bridgeSQLiteLastError;
                case 0x1F511:
                    return bridgeSQLiteLastInsertID;
            }
        case 0x1F4AC:
            switch (symbol) {
                case 0x1F4CD:
                    return bridgeSQLiteBindInteger;
                case 0x1F53D:
                    return bridgeSQLiteStep;
            }
    }
    return NULL;
}

InitializerHandler handlerPointerForInitializer(EmojicodeChar cl, EmojicodeChar symbol){
    switch (cl) {
        case 0x1F4DA:
            return bridgeSQLiteOpen; //0x1F4C2
        case 0x1F4AC:
            return bridgeSQLitePrepare; //0x1F44B
    }
    return NULL;
}

Marker markerPointerForClass(EmojicodeChar cl){
    return NULL;
}

uint_fast32_t sizeForClass(EmojicodeChar cl) {
    switch (cl) {
        case 0x1F4DA:
            return sizeof(sqlite3*);
        case 0x1F4AC:
            return sizeof(sqlite3_stmt*);
    }
    return 0;
}

Deinitializer deinitializerPointerForClass(EmojicodeChar cl){
    switch (cl) {
        case 0x1F4DA:
            return bridgeSQLiteRelease; //0x1F4C2
        case 0x1F4AC:
            return bridgeSQLiteFinalize; //0x1F44B
    }
    return NULL;
}