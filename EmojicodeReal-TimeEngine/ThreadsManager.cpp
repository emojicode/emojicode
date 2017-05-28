//
//  ThreadsManager.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/05/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ThreadsManager.hpp"
#include "Thread.hpp"
#include <atomic>

using Emojicode::Thread;

Thread *lastThread_ = nullptr;
std::atomic_uint threads_(0);
std::mutex Emojicode::ThreadsManager::threadListMutex;

Thread* Emojicode::ThreadsManager::anyThread() {
    return lastThread_;
}

unsigned int Emojicode::ThreadsManager::threadsCount() {
    return threads_;
}

Thread* Emojicode::ThreadsManager::nextThread(Thread *thread) {
    return thread->threadBefore_;
}

Thread* Emojicode::ThreadsManager::allocateThread() {
    std::lock_guard<std::mutex> threadListLock(threadListMutex);
    auto thread = new Thread;
    thread->threadBefore_ = lastThread_;
    thread->threadAfter_ = nullptr;
    if (lastThread_ != nullptr) {
        lastThread_->threadAfter_ = thread;
    }
    lastThread_ = thread;
    threads_++;
    return thread;
}

void Emojicode::ThreadsManager::deallocateThread(Thread *thread) {
    std::lock_guard<std::mutex> threadListLock(threadListMutex);
    Thread *before = thread->threadBefore_;
    Thread *after = thread->threadAfter_;

    if (before != nullptr) {
        before->threadAfter_ = after;
    }
    if (after != nullptr) {
        after->threadBefore_ = before;
    }
    else {
        lastThread_ = before;
    }

    delete thread;
    threads_--;
}
