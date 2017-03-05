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

/**
 * Generates a secure random number. The integer is either generated using arc4random_uniform if available
 * or by reading from @c /dev/urandmon.
 * @param min The minimal integer (inclusive).
 * @param max The maximal integer (inclusive).
 * @deprecated Use @c std::rand instead.
 */
[[deprecated]] extern EmojicodeInteger secureRandomNumber(EmojicodeInteger min, EmojicodeInteger max);

struct Data {
    EmojicodeInteger length;
    char *bytes;
    Object *bytesObject;
};

}

#endif /* standard_h */
