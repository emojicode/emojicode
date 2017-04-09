//
//  CallableWriter.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/09/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CallableWriter_h
#define CallableWriter_h

#include "EmojicodeCompiler.hpp"
#include <stdexcept>
#include <vector>

class CallableWriter;
class Writer;
class WriteLocation;
class RecompilationPoint;

class CallableWriterPlaceholder {
    friend CallableWriter;
public:
    void write(EmojicodeInstruction value) const;
protected:
    CallableWriterPlaceholder(CallableWriter *writer, size_t index) : writer_(writer), index_(index) {}
    CallableWriter *writer_;
private:
    size_t index_;
};

class CallableWriterCoinsCountPlaceholder : private CallableWriterPlaceholder {
    friend CallableWriter;
public:
    void write();
private:
    CallableWriterCoinsCountPlaceholder(CallableWriter *writer, size_t index, size_t count)
        : CallableWriterPlaceholder(writer, index), count_(count) {}
    size_t count_;
};

class CallableWriterInsertionPoint {
    friend CallableWriter;
    friend WriteLocation;
public:
    /// Inserts one value at the insertion point. Subsequent calls will insert values after the values previously
    /// inserted.
    virtual void insert(EmojicodeInstruction value);
    /// Inserts multiple values at the insertion point in the order given.
    /// Subsequent calls will insert values after the values previously inserted.
    virtual void insert(std::initializer_list<EmojicodeInstruction> values);
private:
    CallableWriterInsertionPoint(CallableWriter *writer, size_t index) : writer_(writer), index_(index) {}
    CallableWriterInsertionPoint() {}
    CallableWriter *writer_;
    size_t index_;
};

/**
 * The callable writer is responsible for storing the bytecode generated for a callable. It is normally used in
 * conjunction with a @c FunctionPAG instance.
 */
class CallableWriter {
    friend CallableWriterPlaceholder;
    friend Writer;
    friend CallableWriterInsertionPoint;
    friend RecompilationPoint;
public:
    virtual void writeInstruction(EmojicodeInstruction value);
    virtual void writeInstruction(std::initializer_list<EmojicodeInstruction> values);

    virtual InstructionCount writtenInstructions() { return instructions_.size(); }

    virtual CallableWriterPlaceholder writeInstructionPlaceholder();

    virtual CallableWriterCoinsCountPlaceholder writeInstructionsCountPlaceholderCoin();

    virtual CallableWriterInsertionPoint getInsertionPoint();

    virtual void writeWriter(const CallableWriter &writer) {
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
    WriteLocation(CallableWriterInsertionPoint insertionPoint)
        : useInsertionPoint_(true), insertionPoint_(insertionPoint) {}
    WriteLocation(CallableWriter &writer) : useInsertionPoint_(false), writer_(&writer) {}
    void write(std::initializer_list<EmojicodeInstruction> values) {
        if (useInsertionPoint_) insertionPoint_.insert(values);
        else writer_->writeInstruction(values);
    }
    CallableWriterInsertionPoint insertionPoint() const {
        if (useInsertionPoint_) return insertionPoint_;
        else return writer_->getInsertionPoint();
    }
private:
    bool useInsertionPoint_;
    CallableWriterInsertionPoint insertionPoint_;
    CallableWriter *writer_;
};

#endif /* CallableWriter_h */
