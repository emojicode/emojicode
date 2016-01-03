//
//  EmojicodeDictionary.c
//  Emojicode
//
//  Created by Theo Weidmann on 19/04/15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeCompiler.h"
#include <string.h>

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

Dictionary* newDictionary(){
    Dictionary *dict = malloc(sizeof(Dictionary));
    dict->capacity = 7;
    dict->count = 0;
    dict->slots = calloc(sizeof(DictionaryKVP), dict->capacity);
    return dict;
}

void dictionaryFree(Dictionary *dict, void(*fr)(void *)){
    for (size_t i = 0; i < dict->capacity; i++) {
        if(dict->slots[i].key){
            fr(dict->slots[i].value);
            free(dict->slots[i].key);
        }
    }
    free(dict->slots);
    free(dict);
}

static size_t findSlot(Dictionary *dict, const void *key, size_t kl){
    size_t index = fnv32((char*)key, kl) % dict->capacity;
    
    while (dict->slots[index].key && !(dict->slots[index].kl == kl && memcmp(dict->slots[index].key, key, kl) == 0)) {
        index = (index + 1) % dict->capacity;
    }
    
    return index;
}

void dictionarySet(Dictionary *dict, const void *key, size_t kl, void *value){
    size_t index = findSlot(dict, key, kl);
    if(dict->slots[index].key){
        free(dict->slots[index].key);
    }
    
    void *keyValue = malloc(kl);
    memcpy(keyValue, key, kl);
    dict->slots[index].key = keyValue;
    dict->slots[index].value = value;
    dict->slots[index].kl = kl;

    if(++dict->count > 2 * (dict->capacity/3)){
        size_t oldCapacity = dict->capacity;
        DictionaryKVP *oldSlots = dict->slots;
        
        dict->capacity *= 2;
        dict->count = 0;
        dict->slots = calloc(sizeof(DictionaryKVP), dict->capacity);
        
        for (size_t i = 0; i < oldCapacity; i++) {
            if(oldSlots[i].key){
                size_t index = findSlot(dict, oldSlots[i].key, oldSlots[i].kl);
                dict->slots[index].key = oldSlots[i].key;
                dict->slots[index].value = oldSlots[i].value;
                dict->slots[index].kl = oldSlots[i].kl;
            }
        }
        
        free(oldSlots);
    }
}

void* dictionaryLookup(Dictionary *dict, const void *key, size_t kl){
    size_t index = findSlot(dict, key, kl);
    if(dict->slots[index].key){
        return dict->slots[index].value;
    }
    return NULL;
}

void dictionaryRemove(Dictionary *dict, const void *key, size_t kl){
    size_t index = findSlot(dict, key, kl);
    dict->count--;
    free(dict->slots[index].key);
    dict->slots[index].key = NULL;
}