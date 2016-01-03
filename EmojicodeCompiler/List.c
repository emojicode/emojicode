//
//  List.c
//  Emojicode
//
//  Created by Theo Weidmann on 24.07.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeCompiler.h"
#include <string.h>

/** Shrinks the list if needed */
void shrinkListIfNeeded(List *list);

List* newList(){
    List *list = malloc(sizeof(List));
    list->count = 0;
    list->size = 0;
    list->items = NULL;
    return list;
}

void listRelease(void *l){
    List *list = l;
    free(list->items);
    free(list);
}

void expandListSize(List *list, size_t addition){
    //Addition has currently no use, but it may be used in the future to optimize the new size
    if (list->size == 0) {
        list->items = malloc(2 * sizeof(void *));
        list->size = 2;
    }
    else {
        list->size *= 2;
        list->items = realloc(list->items, sizeof(void *) * list->size);
    }
}

void insertList(List *list, void *o, size_t index){
    if (list->size - list->count == 0) {
        expandListSize(list, 1);
    }
    for (size_t i = list->count - 1; i >= index; i--) {
        list->items[i + 1] = list->items[i];
    }
    list->items[index] = o;
    list->count++;
}

void appendList(List *list, void* o){
    if (list->size - list->count == 0) {
        expandListSize(list, 1);
    }
    list->items[list->count] = o;
    list->count++;
}


bool listRemoveByIndex(List *list, size_t index){
    if (list->count <= index){
        return false;
    }
    for (size_t i = index + 1; i < list->count; i++) {
        list->items[i - 1] = list->items[i];
    }
    list->count--;
    shrinkListIfNeeded(list);
    return true;
}

bool listRemove(List *list, void *x){
    for(size_t i = 0; i < list->count; i++){
        if(list->items[i] == x){
            listRemoveByIndex(list, i);
            return true;
        }
    }
    return false;
}

void* getList(List *list, size_t i){
    if (list->count <= i){
        return NULL;
    }
    return list->items[i];
}

List* listFromList(List *cpdList){
    List *list = malloc(sizeof(List));
    list->count = cpdList->count;
    list->size = cpdList->size;
    list->items = malloc(sizeof(void*) * cpdList->size);
    
    memcpy(list->items, cpdList->items, cpdList->count * sizeof(void*));
    return list;
}

void shrinkListIfNeeded(List *list){
    if(list->count < list->size/2){
        list->size /= 2;
        list->items = realloc(list->items, sizeof(void *) * list->size);
    }
}
