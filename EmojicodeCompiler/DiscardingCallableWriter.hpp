//
//  DiscardingCallableWriter.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 23/10/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef DiscardingCallableWriter_hpp
#define DiscardingCallableWriter_hpp

#include "CallableWriter.hpp"
#include "Token.hpp"

/** A subclass of @c CallableWriter that discards all input. */
class DiscardingCallableWriter : public CallableWriter {
public:
    DiscardingCallableWriter() : CallableWriter() {
        CallableWriter::writeCoin(0, SourcePosition(0, 0, ""));
    }
    
    /** Writes a coin with the given value. */
    virtual void writeCoin(EmojicodeCoin value, SourcePosition p) override {}
    
    virtual size_t writtenCoins() override { return 0; }

    /** Must be used to write any double to the file. */
    virtual void writeDoubleCoin(double val, SourcePosition p) override {}
};

#endif /* DiscardingCallableWriter_hpp */
