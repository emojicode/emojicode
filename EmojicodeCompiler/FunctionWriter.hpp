//
//  FunctionWriter.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/09/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef FunctionWriter_h
#define FunctionWriter_h

#include "EmojicodeCompiler.hpp"
#include <stdexcept>
#include <vector>

namespace EmojicodeCompiler {

class FunctionWriter;
class Writer;
class WriteLocation;
class RecompilationPoint;

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

class FunctionWriterInsertionPoint {
    friend FunctionWriter;
    friend WriteLocation;
public:
    /// Inserts one value at the insertion point. Subsequent calls will insert values after the values previously
    /// inserted.
    virtual void insert(EmojicodeInstruction value);
    /// Inserts multiple values at the insertion point in the order given.
    /// Subsequent calls will insert values after the values previously inserted.
    virtual void insert(std::initializer_list<EmojicodeInstruction> values);
private:
    FunctionWriterInsertionPoint(FunctionWriter *writer, size_t index) : writer_(writer), index_(index) {}
    FunctionWriterInsertionPoint() {}
    FunctionWriter *writer_;
    size_t index_;
};

/**
 * The callable writer is responsible for storing the bytecode generated for a callable. It is normally used in
 * conjunction with a @c FunctionPAG instance.
 */
class FunctionWriter {
    friend FunctionWriterPlaceholder;
    friend Writer;
    friend FunctionWriterInsertionPoint;
    friend RecompilationPoint;
public:
    virtual void writeInstruction(EmojicodeInstruction value);
    virtual void writeInstruction(std::initializer_list<EmojicodeInstruction> values);

    virtual InstructionCount count() { return instructions_.size(); }

    virtual FunctionWriterPlaceholder writeInstructionPlaceholder();

    virtual FunctionWriterCountPlaceholder writeInstructionsCountPlaceholderCoin();

    virtual FunctionWriterInsertionPoint getInsertionPoint();

    virtual void writeWriter(const FunctionWriter &writer) {
        if (&writer == this) throw std::invalid_argument("Attempt to write writer into itself");
        instructions_.insert(instructions_.end(), writer.instructions_.begin(), writer.instructions_.end());
    }

    /** Must be used to write any double to the file. */
    virtual void writeDoubleCoin(double val);
private:
    std::vector<EmojicodeInstruction> instructions_;
};

class WriteLocation {
public:
    WriteLocation(FunctionWriterInsertionPoint insertionPoint)
        : useInsertionPoint_(true), insertionPoint_(insertionPoint) {}
    WriteLocation(FunctionWriter &writer) : useInsertionPoint_(false), writer_(&writer) {}
    void write(std::initializer_list<EmojicodeInstruction> values) {
        if (useInsertionPoint_) insertionPoint_.insert(values);
        else writer_->writeInstruction(values);
    }
    FunctionWriterInsertionPoint insertionPoint() const {
        if (useInsertionPoint_) return insertionPoint_;
        else return writer_->getInsertionPoint();
    }
private:
    bool useInsertionPoint_;
    FunctionWriterInsertionPoint insertionPoint_;
    FunctionWriter *writer_;
};

}  // namespace EmojicodeCompiler

#endif /* FunctionWriter_h */
