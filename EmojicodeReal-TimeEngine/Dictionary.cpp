//
//  EmojicodeDictionary.c
//  Emojicode
//
//  Created by Theo Weidmann on 19/04/15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Dictionary.hpp"
#include "EmojicodeAPI.hpp"
#include "String.hpp"
#include "List.hpp"
#include "Thread.hpp"
#include "Memory.hpp"

namespace Emojicode {

#define FNV_PRIME_64 1099511628211
#define FNV_OFFSET_64 14695981039346656037U
EmojicodeDictionaryHash fnv64(const void *input, size_t length) {
    auto k = static_cast<const char*>(input);
    EmojicodeDictionaryHash hash = FNV_OFFSET_64;
    for (size_t i = 0; i < length; i++) {
        hash = (hash ^ k[i]) * FNV_PRIME_64;
    }
    return hash;
}

EmojicodeDictionaryHash dictionaryHash(Object *key) {
    auto string = key->val<String>();
    return fnv64(string->characters(), (string->length) * sizeof(EmojicodeChar));
}

bool dictionaryKeyEqual(Object *key1, Object *key2) {
    return stringEqual(key1->val<String>(), key2->val<String>());
}

bool dictionaryKeyHashEqual(EmojicodeDictionaryHash hash1, EmojicodeDictionaryHash hash2, Object *key1, Object *key2) {
    return (hash1 == hash2) && dictionaryKeyEqual(key1, key2);
}

// MARK: Internal dictionary

EmojicodeDictionaryNode* dictionaryGetNode(EmojicodeDictionary *dict, Object *key) {
    auto hash = dictionaryHash(key);

    EmojicodeDictionaryNode *e;
    Object** bucko;
    size_t n = 0;
    if (dict->buckets != nullptr) {
        bucko = dict->buckets->val<Object*>();
        if ((n = dict->bucketsCounter) > 0) {
            Object *firsto = bucko[hash & (n - 1)];
            if (firsto != nullptr) {
                e = firsto->val<EmojicodeDictionaryNode>();
                if (dictionaryKeyHashEqual(hash, e->hash, key, e->key)) {
                    return e;
                }
                Object *eo;
                while ((eo = e->next) != nullptr) {
                    e = eo->val<EmojicodeDictionaryNode>();
                    if (dictionaryKeyHashEqual(hash, e->hash, key, e->key)) {
                        return e;
                    }
                }
            }
        }
    }
    return nullptr;
}

Object* dictionaryNewNode(EmojicodeDictionaryHash hash, RetainedObjectPointer key) {
    Object *nodeo = newArray(sizeof(EmojicodeDictionaryNode));
    auto *node = nodeo->val<EmojicodeDictionaryNode>();

    node->hash = hash;
    node->key = key.unretainedPointer();
    node->next = nullptr;
    return nodeo;
}

void dictionaryResize(RetainedObjectPointer dictObject) {
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
        float ft = static_cast<float>(newCap) * dict->loadFactor;
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

Box* dictionaryPutVal(RetainedObjectPointer dictionaryObject, RetainedObjectPointer key, Thread *thread) {
    EmojicodeDictionaryHash hash = dictionaryHash(key.unretainedPointer());

    auto *dictionary = dictionaryObject->val<EmojicodeDictionary>();
    if (dictionary->buckets == nullptr || dictionary->bucketsCounter == 0) {
        dictionaryResize(dictionaryObject);
        dictionary = dictionaryObject->val<EmojicodeDictionary>();
    }

    auto **buckets = dictionary->buckets->val<Object *>();
    size_t n = dictionary->bucketsCounter, i = (hash & (n - 1));

    RetainedObjectPointer destination(nullptr);

    Object *po = buckets[i];
    if (po != nullptr) {
        auto *p = po->val<EmojicodeDictionaryNode>();
        Object *eo = nullptr;
        if (dictionaryKeyHashEqual(hash, p->hash, key.unretainedPointer(), p->key)) {
            eo = po;
        }
        else {
            for (int binCount = 0; ; ++binCount) {
                if (p->next == nullptr) {
                    p->next = dictionaryNewNode(hash, key);
                    destination = thread->retain(p->next);
                    dictionary = dictionaryObject->val<EmojicodeDictionary>();
                    eo = nullptr;
                    break;
                }
                eo = p->next;
                auto *e = eo->val<EmojicodeDictionaryNode>();

                if (dictionaryKeyHashEqual(hash, e->hash, key.unretainedPointer(), e->key)) {
                    break;
                }
                p = e;
            }
        }
        if (eo != nullptr) {  // existing mapping for key
            auto *e = eo->val<EmojicodeDictionaryNode>();
            return &e->value;
        }
    }
    else {
        buckets[i] = dictionaryNewNode(hash, key);
        destination = thread->retain(buckets[i]);
        dictionary = dictionaryObject->val<EmojicodeDictionary>();
    }

    if (++dictionary->size > dictionary->nextThreshold) {
        dictionaryResize(dictionaryObject);
    }
    thread->release(1);
    return &destination->val<EmojicodeDictionaryNode>()->value;
}

void dictionaryRemove(EmojicodeDictionary *dictionary, Object *key) {
    auto hash = dictionaryHash(key);
    size_t n = 0, index = 0;
    if (dictionary->buckets != nullptr && (n = dictionary->bucketsCounter) > 0) {
        auto **bucko = dictionary->buckets->val<Object*>();
        Object *po = bucko[index = hash & (n - 1)];
        if (po != nullptr) {
            auto *p = po->val<EmojicodeDictionaryNode>();
            EmojicodeDictionaryNode *node = nullptr;
            if (dictionaryKeyHashEqual(hash, p->hash, key, p->key)) {
                node = p;
            }
            else {
                Object *nexto = p->next;
                while (nexto != nullptr) {
                    auto *e = nexto->val<EmojicodeDictionaryNode>();
                    if (dictionaryKeyHashEqual(hash, e->hash, key, e->key)) {
                        node = e;
                        break;
                    }
                    p = e;
                    nexto = e->next;
                }
            }
            if (node != nullptr) {
                if (node == p) {
                    bucko[index] = node->next;
                }
                else {
                    p->next = node->next;
                }
                dictionary->size--;
            }
        }
    }
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
    auto dict = object->val<EmojicodeDictionary>();

    if (dict->buckets == nullptr) {
        return;
    }
    mark(&dict->buckets);

    auto buckets = dict->buckets->val<Object*>();
    for (size_t i = 0; i < dict->bucketsCounter; i++) {
        Object **eo = &buckets[i];
        while (*(eo) != nullptr) {
            mark(eo);
            auto *e = (*eo)->val<EmojicodeDictionaryNode>();
            mark(&(e->key));
            markBox(&e->value);
            eo = &(e->next);
        }
    }
}

// MARK: Bridges

void bridgeDictionarySet(Thread *thread) {
    dictionaryPutVal(thread->thisObjectAsRetained(),
                     thread->variableObjectPointerAsRetained(0), thread)->copy(thread->variableDestination(1));
    thread->returnFromFunction();
}

void bridgeDictionaryGet(Thread *thread) {
    Object *key = thread->variable(0).object;
    auto *dictionary = thread->thisObject()->val<EmojicodeDictionary>();
    EmojicodeDictionaryNode *node = dictionaryGetNode(dictionary, key);
    if (node == nullptr) {
        thread->returnNothingnessFromFunction();
    }
    else {
        thread->returnFromFunction(node->value);
    }
}

void bridgeDictionaryRemove(Thread *thread) {
    dictionaryRemove(thread->thisObject()->val<EmojicodeDictionary>(), thread->variable(0).object);
    thread->returnFromFunction();
}

void bridgeDictionaryKeys(Thread *thread) {
    auto listObject = thread->retain(newObject(CL_LIST));

    auto *dict = thread->thisObject()->val<EmojicodeDictionary>();

    listObject->val<List>()->capacity = dict->size;
    Object *items = newArray(sizeof(Box) * dict->size);
    listObject->val<List>()->items = items;

    for (size_t i = 0, l = dict->bucketsCounter; i < l; i++) {
        auto **bucko = thread->thisObject()->val<EmojicodeDictionary>()->buckets->val<Object*>();
        if (bucko[i] != nullptr) {
            auto nodeo = thread->retain(bucko[i]);
            while (true) {
                listAppendDestination(listObject, thread)->copySingleValue(T_OBJECT,
                                                                           nodeo->val<EmojicodeDictionaryNode>()->key);
                auto next = nodeo->val<EmojicodeDictionaryNode>()->next;
                if (next == nullptr) {
                    break;
                }
                nodeo = next;
            }
            thread->release(1);
        }
    }

    thread->release(1);
    thread->returnFromFunction(listObject.unretainedPointer());
}

void bridgeDictionaryClear(Thread *thread) {
    auto c = static_cast<EmojicodeInteger>(dictionaryClear(thread->thisObject()->val<EmojicodeDictionary>()));
    thread->returnFromFunction(c);
}

void bridgeDictionaryContains(Thread *thread) {
    Object *key = thread->variable(0).object;
    auto dictionary = thread->thisObject()->val<EmojicodeDictionary>();
    thread->returnFromFunction(dictionaryGetNode(dictionary, key) != nullptr);
}

void bridgeDictionarySize(Thread *thread) {
    thread->returnFromFunction(static_cast<EmojicodeInteger>(thread->thisObject()->val<EmojicodeDictionary>()->size));
}

void initDictionaryBridge(Thread *thread) {
    dictionaryInit(thread->thisObject()->val<EmojicodeDictionary>());
    thread->returnFromFunction(thread->thisContext());
}

}  // namespace Emojicode
