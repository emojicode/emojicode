//
//  EmojicodeDictionary.c
//  Emojicode
//
//  Created by Theo Weidmann on 19/04/15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeAPI.h"
#include "EmojicodeDictionary.h"
#include "EmojicodeString.h"

#define FNV_PRIME_32 16777619
#define FNV_OFFSET_32 2166136261U
uint32_t fnv32(const char *k, size_t length){
    uint32_t hash = FNV_OFFSET_32;
    for(size_t i = 0; i < length; i++){
        hash = hash ^ k[i];
        hash = hash * FNV_PRIME_32;
    }
    return hash;
}

#define slots(dict) ((EmojicodeDictionaryKVP *)(dict->slots->value))


void dictionaryInit(Thread *thread){
#define initialCapactiy 7
    Object *slots = newArray(sizeof(EmojicodeDictionaryKVP) * (initialCapactiy));
    
    EmojicodeDictionary *dict = stackGetThis(thread)->value;
    dict->capacity = initialCapactiy;
    dict->slots = slots;
#undef initialCapacity
}

#define hashString(keyString) fnv32((char*)characters(keyString), ((keyString)->length) * sizeof(EmojicodeChar))

static size_t findSlot(EmojicodeDictionary *dict, Object *key){
    size_t index = 0;
    
    while (slots(dict)[index].key && !stringEqual(slots(dict)[index].key->value, key->value)) {
        index = (index + 1) % dict->capacity;
    }
    
    return index;
}


void expandDictionary(Object *dicto, Thread *thread){
    EmojicodeDictionary *dict = dicto->value;
    
    size_t newCapacity = dict->capacity * 2;
    Object *slotso = enlargeArray(dict->slots, newCapacity * sizeof(EmojicodeDictionaryKVP));
    
    dict = stackGetThis(thread)->value;
    
    dict->slots = slotso;
    dict->capacity = newCapacity;

}

bool shouldExpandDictionary(EmojicodeDictionary *dict){
    return dict->count > 2 * (dict->capacity / 3);
}

void dictionarySet(Object *dicto, Object *keyString, Something value, Thread *thread){
    EmojicodeDictionary *dict = dicto->value;
    
    size_t index = findSlot(dict, keyString);
    slots(dict)[index].key = keyString;
    slots(dict)[index].value = value;
    ++dict->count;

    if(shouldExpandDictionary(dict)){
        expandDictionary(dicto, thread);
    }
}

Something dictionaryLookup(EmojicodeDictionary *dict, Object *keyString){
    size_t index = findSlot(dict, keyString);
    if(slots(dict)[index].key){
        return slots(dict)[index].value;
    }
    return NOTHINGNESS;
}

void dictionaryRemove(EmojicodeDictionary *dict, Object *keyString){
    size_t index = findSlot(dict, keyString);
    dict->count--;
    slots(dict)[index].key = NULL;
}

void dictionaryMark(Object *object){
    EmojicodeDictionary *dict = object->value;
    if(dict->slots){
        mark(&dict->slots);
    }
    for (size_t i = 0; i < dict->capacity; i++) {
        if(slots(dict)[i].key){
            mark(&slots(dict)[i].key);
            if(isRealObject(slots(dict)[i].value)){
                mark(&slots(dict)[i].value.object);
            }
        }
    }
}

//MARK: Bridges

static Something bridgeDictionarySet(Thread *thread){
    dictionarySet(stackGetThis(thread), stackGetVariable(0, thread).object, stackGetVariable(1, thread), thread);
    return NOTHINGNESS;
}

static Something bridgeDictionaryGet(Thread *thread){
    return dictionaryLookup(stackGetThis(thread)->value, stackGetVariable(0, thread).object);
}

static Something bridgeDictionaryRemove(Thread *thread){
    dictionaryRemove(stackGetThis(thread)->value, stackGetVariable(0, thread).object);
    return NOTHINGNESS;
}

void bridgeDictionaryInit(Thread *thread){
    dictionaryInit(thread);
}

MethodHandler dictionaryMethodForName(EmojicodeChar name){
    switch (name) {
        case 0x1F43D: //🐽
            return bridgeDictionaryGet;
        case 0x1F428: //🐨
            return bridgeDictionaryRemove;
        case 0x1F437: //🐷
            return bridgeDictionarySet;
    }
    return NULL;
}
