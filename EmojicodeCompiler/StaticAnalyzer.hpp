//
//  StaticAnalyzer.h
//  Emojicode
//
//  Created by Theo Weidmann on 05.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef StaticAnalyzer_hpp
#define StaticAnalyzer_hpp

#include <stdio.h>

/**
 * The static analyzer analyses all method and initializer bodies.
 */

/** 
 * Analyzes all eclass
 */
void analyzeClassesAndWrite(FILE *out);

#endif /* _StaticAnalyzer_hpp */
