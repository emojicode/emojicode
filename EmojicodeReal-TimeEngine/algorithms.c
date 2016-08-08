//
//  algorithms.c
//  Emojicode
//
//  Created by Theo Weidmann on 08/08/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "algorithms.h"
#include <string.h>

void* findBytesInBytes(const void *l, size_t l_len, const void *s, size_t s_len) {
    register char *cur, *last;
    const char *cl = (const char *)l;
    const char *cs = (const char *)s;
    
    /* we need something to compare */
    if (l_len == 0 || s_len == 0)
        return NULL;
    
    /* "s" must be smaller or equal to "l" */
    if (l_len < s_len)
        return NULL;
    
    /* special case where s_len == 1 */
    if (s_len == 1)
        return memchr(l, (int)*cs, l_len);
    
    /* the last position where its possible to find "s" in "l" */
    last = (char *)cl + l_len - s_len;
    
    for (cur = (char *)cl; cur <= last; cur++)
        if (cur[0] == cs[0] && memcmp(cur, cs, s_len) == 0)
            return cur;
    
    return NULL;
}