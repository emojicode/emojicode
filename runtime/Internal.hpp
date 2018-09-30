//
// Created by Theo Weidmann on 17.03.18.
//

#ifndef EMOJICODE_INTERNAL_HPP
#define EMOJICODE_INTERNAL_HPP

#include <atomic>

namespace runtime {

/// This namespace contains variables that should not be considered part of the public API
namespace internal {

extern int argc;
extern char **argv;

struct ControlBlock {
    std::atomic_int strongCount{1};
    std::atomic_int weakCount{0};
};

struct Capture {
    ControlBlock *controlBlock;
    void (*deinit)(Capture*);
};

}

}

#endif //EMOJICODE_INTERNAL_HPP
