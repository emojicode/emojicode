//
//  FunctionWriter.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/09/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef FunctionWriter_h
#define FunctionWriter_h

#include "../EmojicodeCompiler.hpp"
#include <stdexcept>
#include <vector>

namespace EmojicodeCompiler {

class FunctionWriter;
class Writer;

class FunctionWriterPlaceholder {
    friend FunctionWriter;
public:
    void write(EmojicodeInstruction value) const;
protected:
    FunctionWriterPlaceholder(FunctionWriter *writer, size_t index) : writer_(writer), index_(index) {}
    FunctionWriter *writer_;
private:
    size_t index_;
};

class FunctionWriterCountPlaceholder : private FunctionWriterPlaceholder {
    friend FunctionWriter;
public:
    void write();
private:
    FunctionWriterCountPlaceholder(FunctionWriter *writer, size_t index, size_t count)
        : FunctionWriterPlaceholder(writer, index), count_(count) {}
    size_t count_;
};

/**
 * The callable writer is responsible for storing the bytecode generated for a callable. It is normally used in
 * conjunction with a @c FunctionPAG instance.
 */
class FunctionWriter final {
    friend Writer;
    friend FunctionWriterPlaceholder;
public:
    void writeInstruction(EmojicodeInstruction value);
    void writeInstruction(std::initializer_list<EmojicodeInstruction> values);

    InstructionCount count() { return instructions_.size(); }

    FunctionWriterPlaceholder writeInstructionPlaceholder();

    FunctionWriterCountPlaceholder writeInstructionsCountPlaceholderCoin();

    /** Must be used to write any double to the file. */
    void writeDoubleCoin(double val);
private:
    std::vector<EmojicodeInstruction> instructions_;
};

}  // namespace EmojicodeCompiler

#endif /* FunctionWriter_h */
