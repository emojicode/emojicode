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


bool dictionaryKeyEqual(EmojicodeDictionary *dict, Something *key1, Something *key2){
    return stringEqual((String*) key1->object->value, (String*) key2->object->value);
}

EmojicodeDictionaryNode* dictionaryGetNode(EmojicodeDictionary *dict, EmojicodeDictionaryHash hash, Something *key){
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

#define hashString(keyString) fnv32((char*)characters(keyString), ((keyString)->length) * sizeof(EmojicodeChar))
EmojicodeDictionaryHash dictionaryHash(EmojicodeDictionary *dict, Something *key){
    // TODO: Implement hash Callable
    return hashString((String *) key->object->value);
}


bool dictionaryContainsKey(EmojicodeDictionary *dict, Something *key){
    return dictionaryGetNode(dict, dictionaryHash(dict, key), key) != NULL;
}

Object* dictionaryNewNode(EmojicodeDictionary *dict, EmojicodeDictionaryHash hash, Something key, Something value, Object *next){
    Object *nodeo = newArray(sizeof(EmojicodeDictionaryNode));
    EmojicodeDictionaryNode *node = (EmojicodeDictionaryNode*) nodeo->value;
    node->hash = hash;
    node->key = &key;
    node->value = &value;
    node->next = next;
    return nodeo;
}

void dictionaryResize(EmojicodeDictionary *dict){
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
    Object *newTaboo = newArray(newCap * sizeof(Object));
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
}

void dictionaryPutVal(EmojicodeDictionary *dict, EmojicodeDictionaryHash hash, Something *key, Something *value, bool onlyIfAbsent){
    Object **tabo;
    Object *po;
    size_t n = 0, i = 0;
    if(dict->table == NULL || dict->buckets == 0){
        dictionaryResize(dict);
        
    }
    tabo = dict->table->value;
    n = dict->buckets;
    if((po = tabo[i = (hash & (n - 1))]) == NULL)
        tabo[i] = dictionaryNewNode(dict, hash, *key, *value, NULL);
    else {
        EmojicodeDictionaryNode *p = po->value;
        Object *eo = NULL;
        if(p->hash == hash && dictionaryKeyEqual(dict, key, p->key))
            eo = po;
        else {
            for(int binCount = 0; ; ++binCount){
                if(p->next == NULL){
                    p->next = dictionaryNewNode(dict, hash, *key, *value, NULL);
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
            Something *oldValue = e->value;
            if(!onlyIfAbsent || oldValue == NULL){
                e->value = value;
            }
            return;
        }
    }
    if(dict->size > dict->nextThreshold)
        dictionaryResize(dict);
    return;
}

void dictionaryPut(EmojicodeDictionary *dict, Something *key, Something *value){
    dictionaryPutVal(dict, dictionaryHash(dict, key), key, value, false);
}

EmojicodeDictionaryNode* dictionaryRemoveNode(EmojicodeDictionary *dict, EmojicodeDictionaryHash hash, Something *key){
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
                EmojicodeDictionaryNode *e = eo->value;
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
                return node;
            }
        }
    }
    return NULL;
}

void dictionaryRemove(EmojicodeDictionary *dict, Something key){
    dictionaryRemoveNode(dict, dictionaryHash(dict, &key), &key);
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

void dictionaryMark(Object *object){
    EmojicodeDictionary *dict = object->value;
    Object *taboo = dict->table;
    if(taboo == NULL){
        return;
    }
    mark(&dict->table);
    Object **tabo = taboo->value;
    Object **eo;
    EmojicodeDictionaryNode *e;
    
    for (size_t i = 0; i < dict->buckets; i++) {
        eo = &tabo[i];
        if(*eo){
            do {
                mark(eo);
                e = (*eo)->value;
                if(isRealObject(*(e->key)))
                    mark(&(e->key->object));
                if(isRealObject(*(e->value)))
                    mark(&(e->value->object));
                eo = &(e->next);
            } while (*eo != NULL);
        }
    }
}

void dictionarySet(Object *dicto, Something key, Something value, Thread *thread){
    EmojicodeDictionary *dict = dicto->value;
    
    dictionaryPut(dict, &key, &value);
}

void dictionaryInit(Thread *thread){
    EmojicodeDictionary *dict = stackGetThis(thread)->value;
    dict->loadFactor = DICTIONARY_DEFAULT_LOAD_FACTOR;
    dict->size = 0;
    dict->table = NULL;
    dict->nextThreshold = 0;
}

Something dictionaryLookup(EmojicodeDictionary *dict, Something key){
    EmojicodeDictionaryNode* node = dictionaryGetNode(dict, dictionaryHash(dict, &key), &key);
    if(node == NULL){ // node not found
        return NOTHINGNESS;
    } else {
        return *node->value;
    }
}

//MARK: Bridges

static Something bridgeDictionarySet(Thread *thread){
    dictionarySet(stackGetThis(thread), stackGetVariable(0, thread), stackGetVariable(1, thread), thread);
    return NOTHINGNESS;
}

static Something bridgeDictionaryGet(Thread *thread){
    return dictionaryLookup(stackGetThis(thread)->value, stackGetVariable(0, thread));
}

static Something bridgeDictionaryRemove(Thread *thread){
    dictionaryRemove(stackGetThis(thread)->value, stackGetVariable(0, thread));
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
