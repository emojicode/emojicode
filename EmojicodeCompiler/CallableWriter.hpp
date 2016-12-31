//
//  CallableWriter.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/09/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CallableWriter_h
#define CallableWriter_h

#include <vector>
#include "EmojicodeCompiler.hpp"

class CallableWriter;
class Writer;

class CallableWriterPlaceholder {
    friend CallableWriter;
public:
    virtual void write(EmojicodeCoin value);
protected:
    CallableWriterPlaceholder(CallableWriter *writer, size_t index) : writer_(writer), index_(index) {}
    CallableWriter *writer_;
private:
    size_t index_;
};

class CallableWriterCoinsCountPlaceholder : private CallableWriterPlaceholder {
    friend CallableWriter;
public:
    virtual void write();
private:
    CallableWriterCoinsCountPlaceholder(CallableWriter *writer, size_t index, size_t count)
        : CallableWriterPlaceholder(writer, index), count_(count) {}
    size_t count_;
};

class CallableWriterInsertionPoint {
    friend CallableWriter;
public:
    virtual void insert(EmojicodeCoin value);
    virtual void insert(std::initializer_list<EmojicodeCoin> values);
private:
    CallableWriterInsertionPoint(CallableWriter *writer, size_t index) : writer_(writer), index_(index) {}
    CallableWriter *writer_;
    size_t index_;
};

/** 
 * The callable writer is responsible for storing the bytecode generated for a callable. It is normally used in
 * conjunction with a @c CallableParserAndGenerator instance.
 */
class CallableWriter {
    friend CallableWriterPlaceholder;
    friend Writer;
    friend CallableWriterInsertionPoint;
public:
    /** Writes a coin with the given value. */
    virtual void writeCoin(EmojicodeCoin value, SourcePosition p);
  
    virtual size_t writtenCoins() { return coins_.size(); }
    
    virtual CallableWriterPlaceholder writeCoinPlaceholder(SourcePosition p);
    
    virtual CallableWriterCoinsCountPlaceholder writeCoinsCountPlaceholderCoin(SourcePosition p);

    virtual CallableWriterInsertionPoint getInsertionPoint();
    
    /** Must be used to write any double to the file. */
    virtual void writeDoubleCoin(double val, SourcePosition p);
private:
    std::vector<EmojicodeCoin> coins_;
};

#endif /* CallableWriter_h */
