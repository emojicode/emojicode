//
//  VTIProvider.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 25/10/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "VTIProvider.hpp"
#include "Function.hpp"

int ValueTypeVTIProvider::next() {
    vtiCount_++;
    return Function::nextFunctionVti();
}
