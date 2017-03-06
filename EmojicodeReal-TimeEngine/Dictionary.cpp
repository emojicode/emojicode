//
//  EmojicodeDictionary.c
//  Emojicode
//
//  Created by Theo Weidmann on 19/04/15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Dictionary.h"
#include "EmojicodeAPI.hpp"
#include "String.h"
#include "Thread.hpp"
#include <functional>

namespace Emojicode {

#define FNV_PRIME_64 1099511628211
#define FNV_OFFSET_64 14695981039346656037U
EmojicodeDictionaryHash fnv64(const char *k, size_t length) {
    EmojicodeDictionaryHash hash = FNV_OFFSET_64;
    for (size_t i = 0; i < length; i++) {
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

bool dictionaryKeyHashEqual(EmojicodeDictionary *dict, EmojicodeDictionaryHash hash1,
                            EmojicodeDictionaryHash hash2, Object *key1, Object *key2) {
    return (hash1 == hash2) && dictionaryKeyEqual(dict, key1, key2);
}

// MARK: Internal dictionary
EmojicodeDictionaryNode* dictionaryGetNode(EmojicodeDictionary *dict, EmojicodeDictionaryHash hash, Object *key) {
    EmojicodeDictionaryNode *e;
    Object** bucko;
    size_t n = 0;
    if (dict->buckets != nullptr) {
        bucko = (Object**) dict->buckets->value;
        if ((n = dict->bucketsCounter) > 0) {
            Object *firsto = bucko[hash & (n - 1)];
            if (firsto != nullptr) {
                e = static_cast<EmojicodeDictionaryNode *>(firsto->value);
                if (dictionaryKeyHashEqual(dict, hash, e->hash, key, e->key)) {
                    return e;
                }
                Object *eo;
                while ((eo = e->next)) {
                    e = static_cast<EmojicodeDictionaryNode *>(eo->value);
                    if (dictionaryKeyHashEqual(dict, hash, e->hash, key, e->key)) {
                        return e;
                    }
                }
            }
        }
    }
    return nullptr;
}

Object* dictionaryNewNode(EmojicodeDictionaryHash hash, Object *key, Box value, Object *next, Thread *thread) {
    Object *nodeo = newArray(sizeof(EmojicodeDictionaryNode));
    EmojicodeDictionaryNode *node = (EmojicodeDictionaryNode *) nodeo->value;

    node->hash = hash;
    node->key = key;
    node->value = value;
    node->next = next;
    return nodeo;
}

void dictionaryResize(Object *const &dictObject, Thread *thread) {
    EmojicodeDictionary *dict = static_cast<EmojicodeDictionary *>(dictObject->value);

    Object *oldBuckoo = dict->buckets;
    size_t oldCap = (oldBuckoo == nullptr) ? 0 : dict->bucketsCounter;
    size_t oldThr = dict->nextThreshold;
    size_t newCap = oldCap << 1, newThr = 0;

    if (oldCap > 0) {
        if (oldCap >= DICTIONARY_MAXIMUM_CAPACTIY) {
            dict->nextThreshold = DICTIONARY_MAXIMUM_CAPACTIY_THRESHOLD;
            return;
        }
        else if (newCap < DICTIONARY_MAXIMUM_CAPACTIY && oldCap >= DICTIONARY_DEFAULT_INITIAL_CAPACITY) {
            newThr = oldThr << 1;  // double threshold
        }
    }
    else if (oldThr > 0) {  // initial capacity was placed in threshold
        newCap = oldThr;
    }
    else {  // zero initial threshold signifies using defaults
        newCap = DICTIONARY_DEFAULT_INITIAL_CAPACITY;
        newThr = (size_t) (DICTIONARY_DEFAULT_LOAD_FACTOR * DICTIONARY_DEFAULT_INITIAL_CAPACITY);
    }

    if (newThr == 0) {
        float ft = (float) newCap * dict->loadFactor;
        newThr = (newCap < DICTIONARY_MAXIMUM_CAPACTIY && ft < (float)DICTIONARY_MAXIMUM_CAPACTIY) ? (size_t)ft : DICTIONARY_MAXIMUM_CAPACTIY_THRESHOLD;
    }

    Object *newBuckoo = newArray(newCap * sizeof(Object *));
    dict = static_cast<EmojicodeDictionary *>(dictObject->value);

    dict->buckets = newBuckoo;
    dict->nextThreshold = newThr;
    dict->bucketsCounter = newCap;

    Object **newBucko = static_cast<Object **>(newBuckoo->value);
    if (oldBuckoo != nullptr) {
        for (int j = 0; j < oldCap; ++j) {
            Object **oldBucko = static_cast<Object **>(oldBuckoo->value);
            Object *eo = oldBucko[j];
            if (eo != nullptr) {
                EmojicodeDictionaryNode *e = static_cast<EmojicodeDictionaryNode *>(eo->value);
                oldBucko[j] = nullptr;
                if (e->next == nullptr) {
                    newBucko[e->hash & (newCap - 1)] = eo;
                }
                else {  // preserve order
                    Object *loHeado = nullptr, *loTailo = nullptr;
                    Object *hiHeado = nullptr, *hiTailo = nullptr;
                    Object *nexto;
                    do {
                        e = static_cast<EmojicodeDictionaryNode *>(eo->value);
                        nexto = e->next;
                        if ((e->hash & oldCap) == 0) {
                            if (loTailo == nullptr) {
                                loHeado = eo;
                            }
                            else {
                                EmojicodeDictionaryNode *loTail = static_cast<EmojicodeDictionaryNode *>(loTailo->value);
                                loTail->next = eo;
                            }
                            loTailo = eo;
                        }
                        else {
                            if (hiTailo == nullptr) {
                                hiHeado = eo;
                            }
                            else {
                                EmojicodeDictionaryNode *hiTail = static_cast<EmojicodeDictionaryNode *>(hiTailo->value);
                                hiTail->next = eo;
                            }
                            hiTailo = eo;
                        }
                    } while ((eo = nexto) != nullptr);

                    if (loTailo != nullptr) {
                        EmojicodeDictionaryNode *loTail = static_cast<EmojicodeDictionaryNode *>(loTailo->value);
                        loTail->next = nullptr;
                        newBucko[j] = loHeado;
                    }
                    if (hiTailo != nullptr) {
                        EmojicodeDictionaryNode *hiTail = static_cast<EmojicodeDictionaryNode *>(hiTailo->value);
                        hiTail->next = nullptr;
                        newBucko[j + oldCap] = hiHeado;
                    }
                }
            }
        }
    }
}

void dictionaryPutVal(Object *dicto, Object *key, Box value, Thread *thread) {
    Object *const &dictionaryObject = thread->retain(dicto);
    EmojicodeDictionaryHash hash = dictionaryHash(static_cast<EmojicodeDictionary *>(dicto->value), key);

    EmojicodeDictionary *dict = static_cast<EmojicodeDictionary *>(dictionaryObject->value);

    if (dict->buckets == nullptr || dict->bucketsCounter == 0) {
        dictionaryResize(dictionaryObject, thread);
        dict = static_cast<EmojicodeDictionary *>(dictionaryObject->value);
    }

    Object **bucko = static_cast<Object **>(dict->buckets->value);
    size_t n = dict->bucketsCounter, i = 0;

    Object *po;

    if ((po = bucko[i = (hash & (n - 1))]) == nullptr) {
        bucko[i] = dictionaryNewNode(hash, key, value, nullptr, thread);
        dict = static_cast<EmojicodeDictionary *>(dictionaryObject->value);
    }
    else {
        EmojicodeDictionaryNode *p = static_cast<EmojicodeDictionaryNode *>(po->value);
        Object *eo = nullptr;
        if (dictionaryKeyHashEqual(dict, hash, p->hash, key, p->key)) {
            eo = po;
        }
        else {
            for (int binCount = 0; ; ++binCount) {
                if (p->next == nullptr) {
                    p->next = dictionaryNewNode(hash, key, value, nullptr, thread);
                    dict = static_cast<EmojicodeDictionary *>(dictionaryObject->value);
                    eo = nullptr;
                    break;
                }
                eo = p->next;
                EmojicodeDictionaryNode *e = static_cast<EmojicodeDictionaryNode *>(eo->value);

                if (dictionaryKeyHashEqual(dict, hash, e->hash, key, e->key)) {
                    break;
                }
                p = e;
            }
        }
        if (eo != nullptr) {  // existing mapping for key
            EmojicodeDictionaryNode *e = static_cast<EmojicodeDictionaryNode *>(eo->value);
            e->value = value;
            return;
        }
    }

    if (++dict->size > dict->nextThreshold) {
        dictionaryResize(dictionaryObject, thread);
    }
    thread->release(1);
}

EmojicodeDictionaryNode* dictionaryRemoveNode(EmojicodeDictionary *dict, EmojicodeDictionaryHash hash, Object *key, Thread *thread) {
    size_t n = 0, index = 0;
    if (dict->buckets != nullptr && (n = dict->bucketsCounter) > 0) {
        Object **bucko = static_cast<Object **>(dict->buckets->value);
        Object *po = bucko[index = hash & (n - 1)];
        if (po != nullptr) {
            EmojicodeDictionaryNode *p = static_cast<EmojicodeDictionaryNode *>(po->value);
            EmojicodeDictionaryNode *node = nullptr;
            if (dictionaryKeyHashEqual(dict, hash, p->hash, key, p->key)) {
                node = p;
            }
            else {
                Object *nexto = p->next;
                while (nexto) {
                    EmojicodeDictionaryNode *e = static_cast<EmojicodeDictionaryNode *>(nexto->value);
                    if (dictionaryKeyHashEqual(dict, hash, e->hash, key, e->key)) {
                        node = e;
                        break;
                    }
                    p = e;
                    nexto = e->next;
                }
            }
            if(node != nullptr) {
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
    return nullptr;
}


// MARK: Bridge -> Dictionary interface
void dictionaryRemove(EmojicodeDictionary *dict, Object *key, Thread *thread) {
    dictionaryRemoveNode(dict, dictionaryHash(dict, key), key, thread);
}

bool dictionaryContains(EmojicodeDictionary *dict, Object *key) {
    return dictionaryGetNode(dict, dictionaryHash(dict, key), key) != nullptr;
}

size_t dictionaryClear(EmojicodeDictionary *dict) {
    size_t sizeBefore = dict->size;
    dict->loadFactor = DICTIONARY_DEFAULT_LOAD_FACTOR;
    dict->size = 0;
    dict->buckets = nullptr;
    dict->nextThreshold = 0;
    return sizeBefore;
}

void dictionaryInit(Thread *thread) {
    EmojicodeDictionary *dict = static_cast<EmojicodeDictionary *>(thread->getThisObject()->value);
    dict->loadFactor = DICTIONARY_DEFAULT_LOAD_FACTOR;
}

void dictionaryMark(Object *object) {
//    EmojicodeDictionary *dict = static_cast<EmojicodeDictionary *>(object->value);
//
//    if(dict->buckets == nullptr){
//        return;
//    }
//    mark(&dict->buckets);
//
//    Object **buckets = static_cast<Object **>(dict->buckets->value);
//    for (size_t i = 0; i < dict->bucketsCounter; i++) {
//        Object **eo = &buckets[i];
//        while (*(eo)) {
//            mark(eo);
//            EmojicodeDictionaryNode *e = static_cast<EmojicodeDictionaryNode *>((*eo)->value);
//            mark(&(e->key));
//            if (isRealObject(e->value)){
//                mark(&(e->value.object));
//            }
//            eo = &(e->next);
//        }
//    }
}

//MARK: Bridges

void bridgeDictionarySet(Thread *thread, Value *destination) {
    dictionaryPutVal(thread->getThisObject(), thread->getVariable(0).object,
                     *reinterpret_cast<Box *>(thread->variableDestination(1)), thread);
}

void bridgeDictionaryGet(Thread *thread, Value *destination) {
    Object *key = thread->getVariable(0).object;
    EmojicodeDictionary *dictionary = static_cast<EmojicodeDictionary *>(thread->getThisObject()->value);
    EmojicodeDictionaryNode *node = dictionaryGetNode(dictionary, dictionaryHash(dictionary, key), key);
    if (node == nullptr) {
        destination->raw = T_NOTHINGNESS;
    }
    else {
        node->value.copyTo(destination);
    }
}

void bridgeDictionaryRemove(Thread *thread, Value *destination) {
    dictionaryRemove(static_cast<EmojicodeDictionary *>(thread->getThisObject()->value), thread->getVariable(0).object, thread);
}

void bridgeDictionaryKeys(Thread *thread, Value *destination) {
    Object *const &listObject = thread->retain(newObject(CL_LIST));

    EmojicodeDictionary *dict = static_cast<EmojicodeDictionary *>(thread->getThisObject()->value);

    List *newList = static_cast<List *>(listObject->value);
    newList->capacity = dict->size;
    Object *items = newArray(sizeof(Value) * dict->size);
    static_cast<List *>(listObject->value)->items = items;

    for (size_t i = 0, l = dict->bucketsCounter; i < l; i++) {
        Object **bucko = (Object **)static_cast<EmojicodeDictionary *>(thread->getThisObject()->value)->buckets->value;
        auto nodeo = std::ref(thread->retain(bucko[i]));
        while (nodeo) {
            listAppendDestination(listObject, thread)->copySingleValue(T_OBJECT,
                                                                       ((EmojicodeDictionaryNode *) nodeo.get()->value)->key);
            thread->release(1);
            nodeo = std::ref(thread->retain(((EmojicodeDictionaryNode *) nodeo.get()->value)->next));
        }
        thread->release(1);
    }

    thread->release(1);
    destination->object = listObject;
}

void bridgeDictionaryClear(Thread *thread, Value *destination) {
    destination->raw = static_cast<EmojicodeInteger>(dictionaryClear(static_cast<EmojicodeDictionary *>(thread->getThisObject()->value)));
}

void bridgeDictionaryContains(Thread *thread, Value *destination) {
    Object *key = thread->getVariable(0).object;
    destination->raw = dictionaryContains(static_cast<EmojicodeDictionary *>(thread->getThisObject()->value), key);
}

void bridgeDictionarySize(Thread *thread, Value *destination) {
    destination->raw = static_cast<EmojicodeInteger>(static_cast<EmojicodeDictionary *>(thread->getThisObject()->value)->size);
}

void initDictionaryBridge(Thread *thread, Value *destination) {
    dictionaryInit(thread);
    *destination = thread->getThisContext();
}

}
