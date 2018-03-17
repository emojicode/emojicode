//
// Created by Theo Weidmann on 17.03.18.
//

#include "../runtime/Runtime.h"
#include <random>

namespace s {

class PRNG : public runtime::Object<PRNG> {
public:
    std::mt19937_64 prng = std::mt19937_64(std::random_device()());
};

extern "C" PRNG* sPrngNew() {
    return PRNG::allocateAndInitType();
}

extern "C" runtime::Integer sPrngGetInteger(PRNG *prng, runtime::Integer from, runtime::Integer to) {
    return std::uniform_int_distribution<runtime::Integer>(from, to)(prng->prng);
}

extern "C" runtime::Real sPrngGetReal(PRNG *prng) {
    return std::uniform_real_distribution<runtime::Real>()(prng->prng);
}

}  // namespace s

SET_META_FOR(s::PRNG, s, 1f3b0)