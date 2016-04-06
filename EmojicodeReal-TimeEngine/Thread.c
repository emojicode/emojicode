//
//  Thread.c
//  Emojicode
//
//  Created by Theo Weidmann on 05/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Emojicode.h"
#include <pthread.h>

Thread *lastThread = NULL;
int threads = 0;
pthread_mutex_t threadListMutex = PTHREAD_MUTEX_INITIALIZER;

Thread* allocateThread() {
#define stackSize (sizeof(StackFrame) + 4 * sizeof(Something)) * 10000 //ca. 400 KB
    Thread *thread = malloc(sizeof(Thread));
    thread->stackLimit = malloc(stackSize);
    thread->returned = false;
    if (!thread->stackLimit) {
        error("Could not allocate stack!");
    }
    thread->futureStack = thread->stack = thread->stackBottom = thread->stackLimit + stackSize - 1;
    
    pthread_mutex_lock(&threadListMutex);
    thread->threadBefore = lastThread;
    thread->threadAfter = NULL;
    if (lastThread) {
        lastThread->threadAfter = thread;
    }
    lastThread = thread;
    threads++;
    pthread_mutex_unlock(&threadListMutex);
    
    return thread;
}

void removeThread(Thread *thread) {
    pthread_mutex_lock(&threadListMutex);
    Thread *before = thread->threadBefore;
    Thread *after = thread->threadAfter;
    
    if (before) before->threadAfter = after;
    if (after) after->threadBefore = before;
    
    threads--;
    pthread_mutex_unlock(&threadListMutex);
    
    free(thread->stackLimit);
    free(thread);
}