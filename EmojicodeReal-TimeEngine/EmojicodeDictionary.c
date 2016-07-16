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

#define FNV_PRIME_64 1099511628211
#define FNV_OFFSET_64 14695981039346656037U
EmojicodeDictionaryHash fnv64(const char *k, size_t length) {
    EmojicodeDictionaryHash hash = FNV_OFFSET_64;
    for(size_t i = 0; i < length; i++){
        hash = hash ^ k[i];
        hash = hash * FNV_PRIME_64;
    }
    return hash;
}

EmojicodeDictionaryHash dictionaryHash(EmojicodeDictionary *dict, Object *key) {
    #define hashString(keyString) fnv64((char*)characters(keyString), ((keyString)->length) * sizeof(EmojicodeChar))
    return hashString((String *) key->value);
}

bool dictionaryKeyEqual(EmojicodeDictionary *dict, Object *key1, Object *key2) {
    return stringEqual((String *) key1->value, (String *) key2->value);
}

bool dictionaryKeyHashEqual(EmojicodeDictionary *dict, EmojicodeDictionaryHash hash1, EmojicodeDictionaryHash hash2, Object *key1, Object *key2) {
    return (hash1 == hash2) && dictionaryKeyEqual(dict, key1, key2);
}

// MARK: Internal dictionary
EmojicodeDictionaryNode* dictionaryGetNode(EmojicodeDictionary *dict, EmojicodeDictionaryHash hash, Object *key) {
    EmojicodeDictionaryNode *e;
    Object** bucko;
    size_t n = 0;
    if (dict->buckets != NULL) {
        bucko = (Object**) dict->buckets->value;
        if ((n = dict->bucketsCounter) > 0) {
            Object *firsto = bucko[hash & (n - 1)];
            if (firsto != NULL) {
                e = firsto->value;
                if (dictionaryKeyHashEqual(dict, hash, e->hash, key, e->key)) {
                    return e;
                }
                Object *eo;
                while ((eo = e->next)) {
                    e = eo->value;
                    if (dictionaryKeyHashEqual(dict, hash, e->hash, key, e->key)) {
                        return e;
                    }
                }
            }
        }
    }
    return NULL;
}

/** @warning GC-Invoking */
Object* dictionaryNewNode(Object **dicto, EmojicodeDictionaryHash hash, Object *key, Something value, Object *next, Thread *thread){
    stackPush(somethingObject(*dicto), 0, 0, thread);
    Object *nodeo = newArray(sizeof(EmojicodeDictionaryNode));
    EmojicodeDictionaryNode *node = (EmojicodeDictionaryNode *) nodeo->value;
    *dicto = stackGetThisObject(thread);
    stackPop(thread);
    
    node->hash = hash;
    node->key = key;
    node->value = value;
    node->next = next;
    return nodeo;
}

/** @warning GC-Invoking */
Object* dictionaryResize(Object *dicto, Thread *thread) {
    EmojicodeDictionary *dict = dicto->value;

    Object *oldBuckoo = dict->buckets;
    size_t oldCap = (oldBuckoo == NULL) ? 0 : dict->bucketsCounter;
    size_t oldThr = dict->nextThreshold;
    size_t newCap = oldCap << 1, newThr = 0;
    
    if (oldCap > 0) {
        if (oldCap >= DICTIONARY_MAXIMUM_CAPACTIY) {
            dict->nextThreshold = DICTIONARY_MAXIMUM_CAPACTIY_THRESHOLD;
            return dicto;
        }
        else if (newCap < DICTIONARY_MAXIMUM_CAPACTIY && oldCap >= DICTIONARY_DEFAULT_INITIAL_CAPACITY) {
            newThr = oldThr << 1; // double threshold
        }
    }
    else if (oldThr > 0) { // initial capacity was placed in threshold
        newCap = oldThr;
    }
    else { // zero initial threshold signifies using defaults
        newCap = DICTIONARY_DEFAULT_INITIAL_CAPACITY;
        newThr = (size_t) (DICTIONARY_DEFAULT_LOAD_FACTOR * DICTIONARY_DEFAULT_INITIAL_CAPACITY);
    }
    
    if (newThr == 0) {
        float ft = (float) newCap * dict->loadFactor;
        newThr = (newCap < DICTIONARY_MAXIMUM_CAPACTIY && ft < (float)DICTIONARY_MAXIMUM_CAPACTIY) ? (size_t)ft : DICTIONARY_MAXIMUM_CAPACTIY_THRESHOLD;
    }
    
    stackPush(somethingObject(dicto), 0, 0, thread);
    Object *newBuckoo = newArray(newCap * sizeof(Object *));
    dicto = stackGetThisObject(thread);
    dict = dicto->value;
    stackPop(thread);
    
    dict->buckets = newBuckoo;
    dict->nextThreshold = newThr;
    dict->bucketsCounter = newCap;
    
    Object **newBucko = newBuckoo->value;
    if (oldBuckoo != NULL) {
        for (int j = 0; j < oldCap; ++j) {
            Object **oldBucko = oldBuckoo->value;
            Object *eo = oldBucko[j];
            if (eo != NULL) {
                EmojicodeDictionaryNode *e = eo->value;
                oldBucko[j] = NULL;
                if (e->next == NULL) {
                    newBucko[e->hash & (newCap - 1)] = eo;
                }
                else { // preserve order
                    Object *loHeado = NULL, *loTailo = NULL;
                    Object *hiHeado = NULL, *hiTailo = NULL;
                    Object *nexto;
                    do {
                        e = eo->value;
                        nexto = e->next;
                        if ((e->hash & oldCap) == 0) {
                            if (loTailo == NULL) {
                                loHeado = eo;
                            }
                            else {
                                EmojicodeDictionaryNode *loTail = loTailo->value;
                                loTail->next = eo;
                            }
                            loTailo = eo;
                        }
                        else {
                            if (hiTailo == NULL) {
                                hiHeado = eo;
                            }
                            else {
                                EmojicodeDictionaryNode *hiTail = hiTailo->value;
                                hiTail->next = eo;
                            }
                            hiTailo = eo;
                        }
                    } while ((eo = nexto) != NULL);
                    
                    if (loTailo != NULL) {
                        EmojicodeDictionaryNode *loTail = loTailo->value;
                        loTail->next = NULL;
                        newBucko[j] = loHeado;
                    }
                    if(hiTailo != NULL) {
                        EmojicodeDictionaryNode *hiTail = hiTailo->value;
                        hiTail->next = NULL;
                        newBucko[j + oldCap] = hiHeado;
                    }
                }
            }
        }
    }
    return dicto;
}

void dictionaryPutVal(Object *dicto, Object *key, Something value, Thread *thread) {
    EmojicodeDictionaryHash hash = dictionaryHash(dicto->value, key);
    
    EmojicodeDictionary *dict = dicto->value;
    
    if (dict->buckets == NULL || dict->bucketsCounter == 0) {
        dicto = dictionaryResize(dicto, thread);
        dict = dicto->value;
    }
    
    Object **bucko;
    size_t n = 0, i = 0;
    bucko = dict->buckets->value;
    n = dict->bucketsCounter;
    
    Object *po;
    
    if ((po = bucko[i = (hash & (n - 1))]) == NULL) {
        bucko[i] = dictionaryNewNode(&dicto, hash, key, value, NULL, thread);
        dict = dicto->value;
    }
    else {
        EmojicodeDictionaryNode *p = po->value;
        Object *eo = NULL;
        if (dictionaryKeyHashEqual(dict, hash, p->hash, key, p->key)) {
            eo = po;
        }
        else {
            for (int binCount = 0; ; ++binCount) {
                if (p->next == NULL) {
                    p->next = dictionaryNewNode(&dicto, hash, key, value, NULL, thread);
                    dict = dicto->value;
                    eo = NULL;
                    break;
                }
                eo = p->next;
                EmojicodeDictionaryNode *e = eo->value;
                
                if (dictionaryKeyHashEqual(dict, hash, e->hash, key, e->key)) {
                    break;
                }
                p = e;
            }
        }
        if (eo != NULL) { // existing mapping for key
            EmojicodeDictionaryNode *e = eo->value;
            e->value = value;
            return;
        }
    }
    if(++(dict->size) > dict->nextThreshold) {
        dicto = dictionaryResize(dicto, thread);
        dict = dicto->value;
    }
}

EmojicodeDictionaryNode* dictionaryRemoveNode(EmojicodeDictionary *dict, EmojicodeDictionaryHash hash, Object *key, Thread *thread) {
    size_t n = 0, index = 0;
    if (dict->buckets != NULL && (n = dict->bucketsCounter) > 0) {
        Object **bucko = dict->buckets->value;
        Object *po = bucko[index = hash & (n - 1)];
        if (po != NULL) {
            EmojicodeDictionaryNode *p = po->value;
            EmojicodeDictionaryNode *node = NULL;
            if (dictionaryKeyHashEqual(dict, hash, p->hash, key, p->key)) {
                node = p;
            }
            else {
                Object *nexto = p->next;
                while (nexto) {
                    EmojicodeDictionaryNode *e = nexto->value;
                    if (dictionaryKeyHashEqual(dict, hash, e->hash, key, e->key)) {
                        node = e;
                        break;
                    }
                    p = e;
                    nexto = e->next;
                }
            }
            if(node != NULL) {
                if (node == p) {
                    bucko[index] = node->next;
                }
                else {
                    p->next = node->next;
                }
                dict->size--;
                return node;
            }
        }
    }
    return NULL;
}


// MARK: Bridge -> Dictionary interface
void dictionaryRemove(EmojicodeDictionary *dict, Object *key, Thread *thread) {
    dictionaryRemoveNode(dict, dictionaryHash(dict, key), key, thread);
}

bool dictionaryContains(EmojicodeDictionary *dict, Object *key) {
    return dictionaryGetNode(dict, dictionaryHash(dict, key), key) != NULL;
}

size_t dictionaryClear(EmojicodeDictionary *dict) {
    size_t sizeBefore = dict->size;
    dict->loadFactor = DICTIONARY_DEFAULT_LOAD_FACTOR;
    dict->size = 0;
    dict->buckets = NULL;
    dict->nextThreshold = 0;
    return sizeBefore;
}

Something dictionaryKeys(Object *dicto, Thread *thread) {
    stackPush(somethingObject(dicto), 2, 0, thread);
    
    {
        Object *listObject = newObject(CL_LIST);
        stackSetVariable(0, somethingObject(listObject), thread);
        
        dicto = stackGetThisObject(thread);
        EmojicodeDictionary *dict = dicto->value;
        
        List *newList = listObject->value;
        newList->capacity = dict->size;
        Object *items = newArray(sizeof(Something) * dict->size);
        ((List *)stackGetVariable(0, thread).object->value)->items = items;
    }
    
    dicto = stackGetThisObject(thread);
    EmojicodeDictionary *dict = dicto->value;
    
    for (size_t i = 0; i < dict->bucketsCounter; i++) {
        Object **bucko = (Object **)dict->buckets->value;
        Object *nodeo = bucko[i];
        while (nodeo) {
            stackSetVariable(1, somethingObject(nodeo), thread);
            
            listAppend(stackGetVariable(0, thread).object, somethingObject(((EmojicodeDictionaryNode *) nodeo->value)->key), thread);

            nodeo = ((EmojicodeDictionaryNode *) stackGetVariable(1, thread).object->value)->next;
            
            dicto = stackGetThisObject(thread);
            dict = dicto->value;
        }
    }
    
    Something listSth = stackGetVariable(0, thread);
    stackPop(thread);
    
    return listSth;
}

void dictionaryInit(Thread *thread) {
    EmojicodeDictionary *dict = stackGetThisObject(thread)->value;
    dict->loadFactor = DICTIONARY_DEFAULT_LOAD_FACTOR;
}

void dictionaryMark(Object *object) {
    EmojicodeDictionary *dict = object->value;
    
    if(dict->buckets == NULL){
        return;
    }
    mark(&dict->buckets);
    
    Object **buckets = dict->buckets->value;
    for (size_t i = 0; i < dict->bucketsCounter; i++) {
        Object **eo = &buckets[i];
        while (*(eo)) {
            mark(eo);
            EmojicodeDictionaryNode *e = (*eo)->value;
            mark(&(e->key));
            if (isRealObject(e->value)){
                mark(&(e->value.object));
            }
            eo = &(e->next);
        }
    }
}

void dictionarySet(Object *dicto, Object *key, Something value, Thread *thread){
    dictionaryPutVal(dicto, key, value, thread);
}

//MARK: Bridges

static Something bridgeDictionarySet(Thread *thread) {
    dictionarySet(stackGetThisObject(thread), stackGetVariable(0, thread).object, stackGetVariable(1, thread), thread);
    return NOTHINGNESS;
}

static Something bridgeDictionaryGet(Thread *thread) {
    Object *key = stackGetVariable(0, thread).object;
    EmojicodeDictionaryNode *node = dictionaryGetNode(stackGetThisObject(thread)->value, dictionaryHash(stackGetThisObject(thread)->value, key), key);
    if(node == NULL){
        return NOTHINGNESS;
    }
    else {
        return node->value;
    }
}

static Something bridgeDictionaryRemove(Thread *thread) {
    dictionaryRemove(stackGetThisObject(thread)->value, stackGetVariable(0, thread).object, thread);
    return NOTHINGNESS;
}

static Something bridgeDictionaryKeys(Thread *thread) {
    return dictionaryKeys(stackGetThisObject(thread), thread);
}

static Something bridgeDictionaryClear(Thread *thread) {
    return somethingInteger(dictionaryClear(stackGetThisObject(thread)->value));
}

static Something bridgeDictionaryContains(Thread *thread) {
    Object *key = stackGetVariable(0, thread).object;
    return somethingBoolean(dictionaryContains(stackGetThisObject(thread)->value, key));
}

static Something bridgeDictionarySize(Thread *thread) {
    return somethingInteger(((EmojicodeDictionary *) stackGetThisObject(thread)->value)->size);
}

void bridgeDictionaryInit(Thread *thread) {
    dictionaryInit(thread);
}

FunctionFunctionPointer dictionaryMethodForName(EmojicodeChar name) {
    switch (name) {
        case 0x1F43D: //🐽
            return bridgeDictionaryGet;
        case 0x1F428: //🐨
            return bridgeDictionaryRemove;
        case 0x1F437: //🐷
            return bridgeDictionarySet;
        case 0x1F419: //🐙
            return bridgeDictionaryKeys;
        case 0x1F417: //🐗
            return bridgeDictionaryClear;
        case 0x1F423: //🐣
            return bridgeDictionaryContains;
        case 0x1F414: //🐔
            return bridgeDictionarySize;
    }
    return NULL;
}
