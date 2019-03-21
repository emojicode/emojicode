//
//  Real.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 09/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include <cmath>
#include <cstdint>
#include <cstdlib>

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

extern "C" double sRealSqrt(double *real) {
    return std::sqrt(*real);
}
