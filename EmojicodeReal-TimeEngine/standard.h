//
//  standard.h
//  Emojicode
//
//  Created by Theo Weidmann on 05/03/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef standard_h
#define standard_h

#include "EmojicodeAPI.hpp"

namespace Emojicode {

struct Data {
    EmojicodeInteger length;
    char *bytes;
    Object *bytesObject;
};

}

#endif /* standard_h */
