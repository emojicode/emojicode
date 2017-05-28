//
//  ThreadsManager.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/05/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ThreadsManager_hpp
#define ThreadsManager_hpp

#include <mutex>

namespace Emojicode {

class Thread;

/// This class is responsible for allocating threads and to give the garbage collector information about the threads
namespace ThreadsManager {
    extern std::mutex threadListMutex;
    Thread* allocateThread();
    void deallocateThread(Thread *thread);
    Thread* anyThread();
    Thread* nextThread(Thread *thread);
    unsigned int threadsCount();
}  // namespace ThreadsManager

}  // namespace Emojicode

#endif /* ThreadsManager_hpp */
