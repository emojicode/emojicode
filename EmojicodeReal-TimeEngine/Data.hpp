//
//  Data.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 08/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Data_hpp
#define Data_hpp

#include "EmojicodeAPI.hpp"

namespace Emojicode {

struct Data {
    EmojicodeInteger length;
    char *bytes;
    Object *bytesObject;
};

void dataEqual(Thread *thread);
void dataSize(Thread *thread);
void dataMark(Object *o);
void dataGetByte(Thread *thread);
void dataToString(Thread *thread);
void dataSlice(Thread *thread);
void dataIndexOf(Thread *thread);
void dataByAppendingData(Thread *thread);

}  // namespace Emojicode

#endif /* Data_hpp */
