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
    #define hashString(keyString) fnv64((char*)keyString->characters(), ((keyString)->length) * sizeof(EmojicodeChar))
    return hashString(key->val<String>());
}

bool dictionaryKeyEqual(EmojicodeDictionary *dict, Object *key1, Object *key2) {
    return stringEqual(key1->val<String>(), key2->val<String>());
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
        bucko = dict->buckets->val<Object*>();
        if ((n = dict->bucketsCounter) > 0) {
            Object *firsto = bucko[hash & (n - 1)];
            if (firsto != nullptr) {
                e = firsto->val<EmojicodeDictionaryNode>();
                if (dictionaryKeyHashEqual(dict, hash, e->hash, key, e->key)) {
                    return e;
                }
                Object *eo;
                while ((eo = e->next)) {
                    e = eo->val<EmojicodeDictionaryNode>();
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
    auto *node = nodeo->val<EmojicodeDictionaryNode>();

    node->hash = hash;
    node->key = key;
    node->value = value;
    node->next = next;
    return nodeo;
}

void dictionaryResize(RetainedObjectPointer dictObject, Thread *thread) {
    auto *dict = dictObject->val<EmojicodeDictionary>();

    Object *oldBuckoo = dict->buckets;
    size_t oldCap = (oldBuckoo == nullptr) ? 0 : dict->bucketsCounter;
    size_t oldThr = dict->nextThreshold;
    size_t newCap = oldCap << 1, newThr = 0;

    if (oldCap > 0) {
        if (oldCap >= DICTIONARY_MAXIMUM_CAPACTIY) {
            dict->nextThreshold = DICTIONARY_MAXIMUM_CAPACTIY_THRESHOLD;
            return;
        }
        if (newCap < DICTIONARY_MAXIMUM_CAPACTIY && oldCap >= DICTIONARY_DEFAULT_INITIAL_CAPACITY) {
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
    dict = dictObject->val<EmojicodeDictionary>();

    dict->buckets = newBuckoo;
    dict->nextThreshold = newThr;
    dict->bucketsCounter = newCap;

    auto **newBucko = newBuckoo->val<Object*>();
    if (oldBuckoo != nullptr) {
        for (size_t j = 0; j < oldCap; ++j) {
            auto **oldBucko = oldBuckoo->val<Object*>();
            Object *eo = oldBucko[j];
            if (eo != nullptr) {
                auto *e = eo->val<EmojicodeDictionaryNode>();
                oldBucko[j] = nullptr;
                if (e->next == nullptr) {
                    newBucko[e->hash & (newCap - 1)] = eo;
                }
                else {  // preserve order
                    Object *loHeado = nullptr, *loTailo = nullptr;
                    Object *hiHeado = nullptr, *hiTailo = nullptr;
                    Object *nexto;
                    do {
                        e = eo->val<EmojicodeDictionaryNode>();
                        nexto = e->next;
                        if ((e->hash & oldCap) == 0) {
                            if (loTailo == nullptr) {
                                loHeado = eo;
                            }
                            else {
                                auto *loTail = loTailo->val<EmojicodeDictionaryNode>();
                                loTail->next = eo;
                            }
                            loTailo = eo;
                        }
                        else {
                            if (hiTailo == nullptr) {
                                hiHeado = eo;
                            }
                            else {
                                auto *hiTail = hiTailo->val<EmojicodeDictionaryNode>();
                                hiTail->next = eo;
                            }
                            hiTailo = eo;
                        }
                    } while ((eo = nexto) != nullptr);

                    if (loTailo != nullptr) {
                        auto *loTail = loTailo->val<EmojicodeDictionaryNode>();
                        loTail->next = nullptr;
                        newBucko[j] = loHeado;
                    }
                    if (hiTailo != nullptr) {
                        auto *hiTail = hiTailo->val<EmojicodeDictionaryNode>();
                        hiTail->next = nullptr;
                        newBucko[j + oldCap] = hiHeado;
                    }
                }
            }
        }
    }
}

void dictionaryPutVal(RetainedObjectPointer dictionaryObject, Object *key, Box value, Thread *thread) {
    EmojicodeDictionaryHash hash = dictionaryHash(dictionaryObject->val<EmojicodeDictionary>(), key);

    auto *dict = dictionaryObject->val<EmojicodeDictionary>();

    if (dict->buckets == nullptr || dict->bucketsCounter == 0) {
        dictionaryResize(dictionaryObject, thread);
        dict = dictionaryObject->val<EmojicodeDictionary>();
    }

    auto **bucko = dict->buckets->val<Object *>();
    size_t n = dict->bucketsCounter, i = 0;

    Object *po;

    if ((po = bucko[i = (hash & (n - 1))]) == nullptr) {
        bucko[i] = dictionaryNewNode(hash, key, value, nullptr, thread);
        dict = dictionaryObject->val<EmojicodeDictionary>();
    }
    else {
        auto *p = po->val<EmojicodeDictionaryNode>();
        Object *eo = nullptr;
        if (dictionaryKeyHashEqual(dict, hash, p->hash, key, p->key)) {
            eo = po;
        }
        else {
            for (int binCount = 0; ; ++binCount) {
                if (p->next == nullptr) {
                    p->next = dictionaryNewNode(hash, key, value, nullptr, thread);
                    dict = dictionaryObject->val<EmojicodeDictionary>();
                    eo = nullptr;
                    break;
                }
                eo = p->next;
                auto *e = eo->val<EmojicodeDictionaryNode>();

                if (dictionaryKeyHashEqual(dict, hash, e->hash, key, e->key)) {
                    break;
                }
                p = e;
            }
        }
        if (eo != nullptr) {  // existing mapping for key
            auto *e = eo->val<EmojicodeDictionaryNode>();
            e->value = value;
            return;
        }
    }

    if (++dict->size > dict->nextThreshold) {
        dictionaryResize(dictionaryObject, thread);
    }
}

EmojicodeDictionaryNode* dictionaryRemoveNode(EmojicodeDictionary *dict, EmojicodeDictionaryHash hash, Object *key, Thread *thread) {
    size_t n = 0, index = 0;
    if (dict->buckets != nullptr && (n = dict->bucketsCounter) > 0) {
        auto **bucko = dict->buckets->val<Object*>();
        Object *po = bucko[index = hash & (n - 1)];
        if (po != nullptr) {
            auto *p = po->val<EmojicodeDictionaryNode>();
            EmojicodeDictionaryNode *node = nullptr;
            if (dictionaryKeyHashEqual(dict, hash, p->hash, key, p->key)) {
                node = p;
            }
            else {
                Object *nexto = p->next;
                while (nexto) {
                    auto *e = nexto->val<EmojicodeDictionaryNode>();
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

void dictionaryInit(EmojicodeDictionary *dict) {
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

void bridgeDictionarySet(Thread *thread) {
    dictionaryPutVal(thread->thisObjectAsRetained(), thread->getVariable(0).object,
                     *reinterpret_cast<Box *>(thread->variableDestination(1)), thread);
    thread->returnFromFunction();
}

void bridgeDictionaryGet(Thread *thread) {
    Object *key = thread->getVariable(0).object;
    auto *dictionary = thread->getThisObject()->val<EmojicodeDictionary>();
    EmojicodeDictionaryNode *node = dictionaryGetNode(dictionary, dictionaryHash(dictionary, key), key);
    if (node == nullptr) {
        thread->returnNothingnessFromFunction();
    }
    else {
        node->value.copyTo(thread->currentStackFrame()->destination);
        thread->returnFromFunction();
    }
}

void bridgeDictionaryRemove(Thread *thread) {
    dictionaryRemove(thread->getThisObject()->val<EmojicodeDictionary>(), thread->getVariable(0).object, thread);
    thread->returnFromFunction();
}

void bridgeDictionaryKeys(Thread *thread) {
    auto listObject = thread->retain(newObject(CL_LIST));

    auto *dict = thread->getThisObject()->val<EmojicodeDictionary>();

    listObject->val<List>()->capacity = dict->size;
    Object *items = newArray(sizeof(Box) * dict->size);
    listObject->val<List>()->items = items;

    for (size_t i = 0, l = dict->bucketsCounter; i < l; i++) {
        auto **bucko = thread->getThisObject()->val<EmojicodeDictionary>()->buckets->val<Object*>();
        auto nodeo = thread->retain(bucko[i]);
        while (true) {
            listAppendDestination(listObject, thread)->copySingleValue(T_OBJECT,
                                                                       nodeo->val<EmojicodeDictionaryNode>()->key);
            thread->release(1);
            auto next = nodeo->val<EmojicodeDictionaryNode>()->next;
            if (next == nullptr) {
                break;
            }
            nodeo = thread->retain(next);
        }
        thread->release(1);
    }

    thread->release(1);
    thread->returnFromFunction(listObject.unretainedPointer());
}

void bridgeDictionaryClear(Thread *thread) {
    auto c = static_cast<EmojicodeInteger>(dictionaryClear(thread->getThisObject()->val<EmojicodeDictionary>()));
    thread->returnFromFunction(c);
}

void bridgeDictionaryContains(Thread *thread) {
    Object *key = thread->getVariable(0).object;
    thread->returnFromFunction(dictionaryContains(thread->getThisObject()->val<EmojicodeDictionary>(), key));
}

void bridgeDictionarySize(Thread *thread) {
    thread->returnFromFunction(static_cast<EmojicodeInteger>(thread->getThisObject()->val<EmojicodeDictionary>()->size));
}

void initDictionaryBridge(Thread *thread) {
    dictionaryInit(thread->getThisObject()->val<EmojicodeDictionary>());
    thread->returnFromFunction(thread->getThisContext());
}

}
