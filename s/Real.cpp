//
//  Real.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 09/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include <cstdint>
#include <cstdlib>
#include <cmath>

extern "C" double sRealAbsolute(double *real) {
    return std::abs(*real);
}

extern "C" double sRealSin(double *real) {
    return std::sin(*real);
}

extern "C" double sRealCos(double *real) {
    return std::cos(*real);
}

extern "C" double sRealTan(double *real) {
    return std::tan(*real);
}

extern "C" double sRealASin(double *real) {
    return std::asin(*real);
}

extern "C" double sRealACos(double *real) {
    return std::acos(*real);
}

extern "C" double sRealATan(double *real) {
    return std::tan(*real);
}

extern "C" double sRealPower(double *real, double exp) {
    return std::pow(*real, exp);
}

extern "C" double sRealSqrt(double *real) {
    return std::sqrt(*real);
}

extern "C" int64_t sRealFloor(double *real) {
    return std::floor(*real);
}

extern "C" int64_t sRealCeil(double *real) {
    return std::ceil(*real);
}

extern "C" int64_t sRealRound(double *real) {
    return std::round(*real);
}

extern "C" double sRealLog2(double *real) {
    return std::log2(*real);
}

extern "C" double sRealLn(double *real) {
    return std::log(*real);
}

extern "C" double sRealToString(double *real) {
    return std::log(*real);
}
