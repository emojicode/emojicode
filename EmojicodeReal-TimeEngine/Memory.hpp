//
//  Object.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 06/02/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Object_hpp
#define Object_hpp

#include "Engine.hpp"

namespace Emojicode {

#ifndef heapSize
#define heapSize (512 * 1024 * 1024)  // 512 MB
#endif

inline size_t alignSize(size_t size) {
    return size + alignof(Object) - (size % alignof(Object));
}

/// This method is called during the initialization of the Engine.
/// @warning Obviously, you should not call it anywhere else!
void allocateHeap();

/// This method pauses the thread as if the garbage collector requested it.
/// @warning You should normally not call this method.
inline void performPauseForGC();

template <typename T>
inline void markByObjectVariableRecord(ObjectVariableRecord &record, Value *va, T &index) {
    switch (record.type) {
        case ObjectVariableType::Simple:
            if (va[record.variableIndex].object) {
                mark(&va[record.variableIndex].object);
            }
            break;
        case ObjectVariableType::Condition:
            if (va[record.condition].raw)
                mark(&va[record.variableIndex].object);
            break;
        case ObjectVariableType::Box:
            if (va[record.variableIndex].raw == T_OBJECT || (va[record.variableIndex].raw & REMOTE_MASK) != 0)
                mark(&va[record.variableIndex + 1].object);
            break;
        case ObjectVariableType::ConditionalSkip:
            if (!va[record.condition].raw)
                index += record.variableIndex;
            break;
    }
}

void markValueReference(Value **valuePointer);
void markBox(Box *box);

}

#endif /* Object_hpp */
