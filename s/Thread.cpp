//
// Created by Theo Weidmann on 17.03.18.
//

#include "../runtime/Runtime.h"
#include <mutex>
#include <thread>

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
    auto thread = Thread::init();
    callable.retain();
    thread->retain();
    thread->thread = std::thread([thread, callable]() {
        callable();
        callable.release();
        thread->release();
    });
    return thread;
}

extern "C" void sThreadJoin(Thread *thread) {
    thread->thread.join();
}

extern "C" void sThreadDestruct(Thread *thread) {
    thread->~Thread();
}

extern "C" void sThreadDelay(runtime::ClassInfo *, runtime::Integer mcs) {
    std::this_thread::sleep_for(std::chrono::microseconds(mcs));
}

extern "C" Mutex* sMutexNew() {
    return Mutex::init();
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

extern "C" void sMutexDestruct(Mutex *mutex) {
    mutex->~Mutex();
}

}  // namespace s

SET_INFO_FOR(s::Thread, s, 1f488)
SET_INFO_FOR(s::Mutex, s, 1f510)
