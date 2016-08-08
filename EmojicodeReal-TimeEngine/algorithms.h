//
//  algorithms.h
//  Emojicode
//
//  Created by Theo Weidmann on 08/08/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef algorithms_h
#define algorithms_h

#include <stddef.h>

/** @c memmem implementation. */
void* findBytesInBytes(const void *l, size_t l_len, const void *s, size_t s_len);

#endif /* algorithms_h */
