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

#define DICT_DEBUG if(false)

#define FNV_PRIME_64 1099511628211
#define FNV_OFFSET_64 14695981039346656037U
EmojicodeDictionaryHash fnv64(const char *k, size_t length){
    EmojicodeDictionaryHash hash = FNV_OFFSET_64;
    for(size_t i = 0; i < length; i++){
        hash = hash ^ k[i];
        hash = hash * FNV_PRIME_64;
    }
    return hash;
}


bool dictionaryKeyEqual(EmojicodeDictionary *dict, Something key1, Something key2){
    return stringEqual((String*) key1.object->value, (String*) key2.object->value);
}
void printNode(EmojicodeDictionaryNode *node){
    if(node->key.type == 0 && node->value.type == 0) // are strings
        if(node->key.object && node->value.object)
            if (node->key.object->value && node->value.object->value)
                printf("node: k=%s v=%s next=%p\n", stringToChar(node->key.object->value), stringToChar(node->value.object->value), node->next);
}
void printDict(EmojicodeDictionary *dict){
    printf("Dictionary at %p buckets=%zu loadFactor=%.2f size=%zu nextThreshold=%zu table=%p\n", dict, dict->buckets, dict->loadFactor, dict->size, dict->nextThreshold, dict->table);
    if(!dict->table) return;
    printf("\n[\n");
    Object **tabo = dict->table->value;
    for(size_t i = 0; i < dict->buckets; ++i){
        Object *po = tabo[i];
        EmojicodeDictionaryNode *p = NULL;
        printf("\ti=%zu at=%p: ", i, po);
        do{
            if(po){
                p = (EmojicodeDictionaryNode*) po->value;
                printNode((p =po->value));
                if(p->next) printf("\t\t");
            }else
                printf("\n");
        } while (p != NULL && (po = p->next) != NULL );
    }
    printf("]\n\n");
}

EmojicodeDictionaryNode* dictionaryGetNode(EmojicodeDictionary *dict, EmojicodeDictionaryHash hash, Something key){
    DICT_DEBUG printf("DictionaryGetNode hash=%llu key=%s\n", hash, stringToChar(key.object->value));
    EmojicodeDictionaryNode *first, *e;
    Object** tabo;
    size_t n = 0;
    if(dict->table != NULL){
        tabo = (Object**) dict->table->value;
        if((n = dict->buckets) > 0) {
            Object *firsto = tabo[hash & (n - 1)];
            if (firsto != NULL){
                first = firsto->value;
                if(first->hash == hash && dictionaryKeyEqual(dict, key, first->key))
                    return first;
                Object *eo = first->next;
                if(eo != NULL){
                    do {
                        e = eo->value;
                        if(e->hash == hash && dictionaryKeyEqual(dict, key, e->key))
                            return e;
                    } while ((eo = e->next) != NULL);
                }
            }
        }
    }
    return NULL;
}

#define hashString(keyString) fnv64((char*)characters(keyString), ((keyString)->length) * sizeof(EmojicodeChar))
EmojicodeDictionaryHash dictionaryHash(EmojicodeDictionary *dict, Something key){
    // TODO: Implement hash Callable
    return hashString((String *) key.object->value);
}


bool dictionaryContainsKey(EmojicodeDictionary *dict, Something key){
    return dictionaryGetNode(dict, dictionaryHash(dict, key), key) != NULL;
}

Object* dictionaryNewNode(EmojicodeDictionary *dict, EmojicodeDictionaryHash hash, Something key, Something value, Object *next, Thread *thread){
    Object *nodeo = newArray(sizeof(EmojicodeDictionaryNode));
    EmojicodeDictionaryNode *node = (EmojicodeDictionaryNode*) nodeo->value;
    node->hash = hash;
    node->key = key;
    node->value = value;
    node->next = next;
    DICT_DEBUG printf("Creating new node ");
    DICT_DEBUG printNode(node);
    return nodeo;
}

void dictionaryResize(EmojicodeDictionary *dict, Thread *thread){
    DICT_DEBUG printf("Resizing ");
    DICT_DEBUG printDict(dict);
    Object *oldTaboo = dict->table;
    size_t oldCap = (oldTaboo == NULL) ? 0 : dict->buckets;
    size_t oldThr = dict->nextThreshold;
    size_t newCap = oldCap << 1, newThr = 0;
    if (oldCap > 0) {
        if (oldCap >= DICTIONARY_MAXIMUM_CAPACTIY) {
            dict->nextThreshold = DICTIONARY_MAXIMUM_CAPACTIY_THRESHOLD;
            return;
        }
        else if(newCap < DICTIONARY_MAXIMUM_CAPACTIY &&
                oldCap >= DICTIONARY_DEFAULT_INITIAL_CAPACITY)
            newThr = oldThr << 1; // double threshold
    }
    else if (oldThr > 0) // initial capacity was placed in threshold
        newCap = oldThr;
    else { // zero initial threshold signifies using defaults
        newCap = DICTIONARY_DEFAULT_INITIAL_CAPACITY;
        newThr = (size_t) (DICTIONARY_DEFAULT_LOAD_FACTOR * DICTIONARY_DEFAULT_INITIAL_CAPACITY);
    }
    if (newThr == 0){
        float ft = (float) newCap * dict->loadFactor;
        newThr = (newCap < DICTIONARY_MAXIMUM_CAPACTIY && ft < (float) DICTIONARY_MAXIMUM_CAPACTIY ?
                  (size_t) ft : DICTIONARY_MAXIMUM_CAPACTIY_THRESHOLD);
    }
    dict->nextThreshold = newThr;
    dict->buckets = newCap;
    Object *newTaboo = newArray(newCap * sizeof(Object *));
    dict->table = newTaboo;
    Object **newTabo = newTaboo->value;
    if(oldTaboo != NULL){
        for (int j = 0; j < oldCap; ++j){
            Object **oldTabo = oldTaboo->value;
            Object *eo = oldTabo[j];
            if(eo != NULL){
                EmojicodeDictionaryNode *e = eo->value;
                oldTabo[j] = NULL;
                if(e->next == NULL){
                    newTabo[e->hash & (newCap - 1)] = eo;
                } else { // preserve order
                    Object *loHeado = NULL, *loTailo = NULL;
                    Object *hiHeado = NULL, *hiTailo = NULL;
                    Object *nexto;
                    do {
                        e = (EmojicodeDictionaryNode*) eo->value;
                        nexto = e->next;
                        if ((e->hash & oldCap) == 0){
                            if (loTailo == NULL)
                                loHeado = eo;
                            else {
                                EmojicodeDictionaryNode *loTail = loTailo->value;
                                loTail->next = eo;
                            }
                            loTailo = eo;
                        }
                        else {
                            if (hiTailo == NULL)
                                hiHeado = eo;
                            else {
                                EmojicodeDictionaryNode *hiTail = hiTailo->value;
                                hiTail->next = eo;
                            }
                            hiTailo = eo;
                        }
                    } while ((eo = nexto) != NULL);
                    if (loTailo != NULL){
                        EmojicodeDictionaryNode *loTail = loTailo->value;
                        loTail->next = NULL;
                        newTabo[j] = loHeado;
                    }
                    if(hiTailo != NULL) {
                        EmojicodeDictionaryNode *hiTail = hiTailo->value;
                        hiTail->next = NULL;
                        newTabo[j + oldCap] = hiHeado;
                    }
                }
            }
        }
    }
    DICT_DEBUG printf("Resized dict at=%p\n", dict);
}

void dictionaryPutVal(EmojicodeDictionary *dict, EmojicodeDictionaryHash hash, Something key, Something value, Thread *thread){
    // DICT_DEBUG printf("DictionaryPutVal hash=%llu key=%s value=%s\n", hash, stringToChar(key.object->value), stringToChar(value.object->value));
    Object **tabo;
    Object *po;
    size_t n = 0, i = 0;
    if(dict->table == NULL || dict->buckets == 0){
        dictionaryResize(dict, thread);
        
    }
    tabo = dict->table->value;
    n = dict->buckets;
    if((po = tabo[i = (hash & (n - 1))]) == NULL)
        tabo[i] = dictionaryNewNode(dict, hash, key, value, NULL, thread);
    else {
        EmojicodeDictionaryNode *p = po->value;
        Object *eo = NULL;
        if(p->hash == hash && dictionaryKeyEqual(dict, key, p->key))
            eo = po;
        else {
            for(int binCount = 0; ; ++binCount){
                if(p->next == NULL){
                    p->next = dictionaryNewNode(dict, hash, key, value, NULL, thread);
                    break;
                }
                EmojicodeDictionaryNode *e = (EmojicodeDictionaryNode*) p->next->value;
                
                if(e->hash == hash && dictionaryKeyEqual(dict, key, e->key)){
                    break;
                }
                p = e;
            }
        }
        if(eo != NULL){ // existing mapping for key
            EmojicodeDictionaryNode *e = (EmojicodeDictionaryNode*) eo->value;
            e->value = value;
            return;
        }
    }
    if(++(dict->size) > dict->nextThreshold)
        dictionaryResize(dict, thread);
    return;
}

void dictionaryPut(EmojicodeDictionary *dict, Something key, Something value, Thread *thread){
    dictionaryPutVal(dict, dictionaryHash(dict, key), key, value, thread);
    DICT_DEBUG printDict(dict);
}

EmojicodeDictionaryNode* dictionaryRemoveNode(EmojicodeDictionary *dict, EmojicodeDictionaryHash hash, Something key, Thread *thread){
    DICT_DEBUG printf("DictionaryRemoveNode hash=%llu key=%s\n", hash, stringToChar(key.object->value));
    size_t n = 0, index = 0;
    if (dict->table != NULL && (n = dict->buckets) > 0) {
        Object **tabo = dict->table->value;
        Object *po = tabo[index = hash & (n - 1)];
        if(po != NULL){
            EmojicodeDictionaryNode *p = po->value;
            EmojicodeDictionaryNode *node = NULL;
            Object *eo;
            if (p->hash == hash && (dictionaryKeyEqual(dict, key, p->key))){
                node = p;
            } else if ((eo = p->next) != NULL){
                EmojicodeDictionaryNode *e;
                do {
                    e = eo->value;
                    if (e->hash == hash && dictionaryKeyEqual(dict, key, e->key)){
                        node = e;
                        break;
                    }
                    p = e;
                } while ((eo = e->next) != NULL);
            }
            if(node != NULL) {
                if (node == p){
                    tabo[index] = node->next;
                } else {
                    p->next = node->next;
                }
                dict->size--;
                return node;
            }
        }
    }
    return NULL;
}

void dictionaryRemove(EmojicodeDictionary *dict, Something key, Thread *thread){
    dictionaryRemoveNode(dict, dictionaryHash(dict, key), key, thread);
}

void dictionaryClear(EmojicodeDictionary *dict){
    if (dict->table != NULL && dict->size > 0){
        EmojicodeDictionaryNode **tab = dict->table->value;
        dict->size = 0;
        for (int i = 0; i < dict->buckets; ++i){
            tab[i] = NULL;
        }
    }
}

#define DICT_DEBUG_MARK DICT_DEBUG
void dictionaryMark(Object *object){
    EmojicodeDictionary *dict = object->value;
    Object *taboo = dict->table;
    if(taboo == NULL){
        return;
    }
    DICT_DEBUG_MARK printf("DictionaryMark ");
    DICT_DEBUG_MARK printDict(dict);
    mark(&dict->table);
    Object **tabo = taboo->value;
    Object **eo;
    EmojicodeDictionaryNode *e;
    
    for (size_t i = 0; i < dict->buckets; i++) {
        eo = &tabo[i];
        if(*eo){
            do {
                DICT_DEBUG_MARK printf("Marking i=%zu ", i);
                DICT_DEBUG_MARK printNode((*eo)->value);
                mark(eo);
                e = (*eo)->value;
                if(isRealObject(e->key)){
                    DICT_DEBUG_MARK printf("a");
                    mark(&(e->key.object));
                }
                if(isRealObject(e->value)){
                    DICT_DEBUG_MARK printf("b");
                    mark(&(e->value.object));
                }
                eo = &(e->next);
            } while (*eo != NULL);
        }
    }
}

void dictionarySet(Object *dicto, Something key, Something value, Thread *thread){
    EmojicodeDictionary *dict = dicto->value;
    
    dictionaryPut(dict, key, value, thread);
}

void dictionaryInit(Thread *thread){
    EmojicodeDictionary *dict = stackGetThis(thread)->value;
    dict->loadFactor = DICTIONARY_DEFAULT_LOAD_FACTOR;
    dict->size = 0;
    dict->table = NULL;
    dict->nextThreshold = 0;
}

Something dictionaryLookup(EmojicodeDictionary *dict, Something key, Thread *thread){
    EmojicodeDictionaryNode* node = dictionaryGetNode(dict, dictionaryHash(dict, key), key);
    if(node == NULL){ // node not found
        return NOTHINGNESS;
    } else {
        return node->value;
    }
}

//MARK: Bridges

static Something bridgeDictionarySet(Thread *thread){
    dictionarySet(stackGetThis(thread), stackGetVariable(0, thread), stackGetVariable(1, thread), thread);
    return NOTHINGNESS;
}

static Something bridgeDictionaryGet(Thread *thread){
    return dictionaryLookup(stackGetThis(thread)->value, stackGetVariable(0, thread), thread);
}

static Something bridgeDictionaryRemove(Thread *thread){
    dictionaryRemove(stackGetThis(thread)->value, stackGetVariable(0, thread), thread);
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
