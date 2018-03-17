//
// Created by Theo Weidmann on 17.03.18.
//

#include "../runtime/Runtime.h"
#include <thread>
#include <mutex>

namespace s {

class Thread : public runtime::Object<Thread> {
public:
    std::thread thread;
};

class Mutex : public runtime::Object<Mutex> {
public:
    std::mutex mutex;
};

extern "C" Thread* sThreadNew(runtime::Callable<void> callable) {
    auto thread = Thread::allocateAndInitType();
    thread->thread = std::thread([callable]() {
        callable();
    });
    return thread;
}

extern "C" void sThreadJoin(Thread *thread) {
    thread->thread.join();
}

extern "C" void sThreadDelay(runtime::ClassType *, runtime::Integer mcs) {
    std::this_thread::sleep_for(std::chrono::microseconds(mcs));
}

extern "C" Mutex* sMutexNew() {
    return Mutex::allocateAndInitType();
}

extern "C" void sMutexLock(Mutex *mutex) {
    mutex->mutex.lock();
}

extern "C" void sMutexTryLock(Mutex *mutex) {
    mutex->mutex.try_lock();
}

extern "C" void sMutexUnlock(Mutex *mutex) {
    mutex->mutex.unlock();
}

}  // namespace s

SET_META_FOR(s::Thread, s, 1f488)
SET_META_FOR(s::Mutex, s, 1f510)