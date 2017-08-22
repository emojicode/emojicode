  //
//  CodeGenerator.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 19/09/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CodeGenerator_hpp
#define CodeGenerator_hpp

#include "Writer.hpp"

namespace EmojicodeCompiler {

/** Generates the bytecode for the program and writes with the given writer. */
void generateCode(Writer *writer);

}  // namespace EmojicodeCompiler

#endif /* CodeGenerator_hpp */
