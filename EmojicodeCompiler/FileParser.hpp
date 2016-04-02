//
//  ClassParser.h
//  Emojicode
//
//  Created by Theo Weidmann on 05.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef ClassParser_hpp
#define ClassParser_hpp

#include "EmojicodeCompiler.hpp"
#include "Package.hpp"

/**
 * The class parsers reads the source code file that only consists of type definitions (eclass and protocols).
 */

/**
 * Parses the given file.
 * @param path Path to the file.
 * @param pkg The name of the package being parsed.
 */
void parseFile(const char *path, Package *pkg);

#endif /* ClassParser_hpp */
