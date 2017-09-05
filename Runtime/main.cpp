//
//  main.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 05/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include <cstdio>
#include <cstdint>
#include <cstdlib>

extern "C" int64_t fn_1f3c1();

extern "C" int8_t* ejcRunTimeAlloc(int64_t size) {
    return static_cast<int8_t*>(malloc(size));
}

int main(int argc, char **argv) {
    auto code = fn_1f3c1();
    printf("%llu\n", code);
    return static_cast<int>(code);
}
