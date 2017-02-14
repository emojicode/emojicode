//
//  Reader.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 06/02/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Reader_hpp
#define Reader_hpp

#include "Engine.hpp"

/// Reads a bytecode file
Function* readBytecode(FILE *in);

/** Determines whether the loading of a package was succesfull */
enum PackageLoadingState {
    PACKAGE_LOADING_FAILED, PACKAGE_HEADER_NOT_FOUND, PACKAGE_INAPPROPRIATE_MAJOR, PACKAGE_INAPPROPRIATE_MINOR,
    PACKAGE_LOADED
};

#endif /* Reader_hpp */
