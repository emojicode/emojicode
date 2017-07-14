//
//  DiscardingFunctionWriter.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 23/10/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef DiscardingFunctionWriter_hpp
#define DiscardingFunctionWriter_hpp

#include "FunctionWriter.hpp"
#include "Token.hpp"

namespace EmojicodeCompiler {

/** A subclass of @c FunctionWriter that discards all input. */
class DiscardingFunctionWriter : public FunctionWriter {
public:
    DiscardingFunctionWriter() : FunctionWriter() {
        FunctionWriter::writeInstruction(0);
    }
    
//    /** Writes a coin with the given value. */
//    virtual void writeInstruction(EmojicodeInstruction value, SourcePosition p) override {}
//    virtual void writeInstruction(EmojicodeInstruction values...) override {}
//    
//    virtual size_t count() override { return 0; }
//
//    /** Must be used to write any double to the file. */
//    virtual void writeDoubleCoin(double val, SourcePosition p) override {}
};

}  // namespace EmojicodeCompiler

#endif /* DiscardingFunctionWriter_hpp */
