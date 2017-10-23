//
//  Integer.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 09/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include <cstdint>
#include <cstdlib>

extern "C" int64_t sIntAbsolute(int64_t *integer) {
    return std::abs(*integer);
}
