//
//  Writer.h
//  Emojicode
//
//  Created by Theo Weidmann on 01.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef Writer_hpp
#define Writer_hpp

#include "EmojicodeCompiler.hpp"

template <typename T>
class WriterPlaceholder;
class WriterCoinsCountPlaceholder;
class Function;

/**
 * The writer finally writes all types to the byte file.
 */
class Writer {
    friend WriterCoinsCountPlaceholder;
    friend WriterPlaceholder<uint16_t>;
    friend WriterPlaceholder<EmojicodeCoin>;
    friend WriterPlaceholder<unsigned char>;
public:
    Writer(FILE *outFile) : out(outFile) {};
    
    /** Must be used to write any uint16_t to the file */
    void writeUInt16(uint16_t value);
    
    /** Writes a coin with the given value */
    void writeCoin(EmojicodeCoin value);
    
    /** Writes a single unicode character */
    void writeEmojicodeChar(EmojicodeChar c);

    void writeByte(unsigned char);
    
    void writeBytes(const char *bytes, size_t count);
    
    void writeFunction(Function *f);
    
    /**
     * Writes a placeholder coin. To replace the placeholder use `writeCoinAtPlaceholder`
     */
    template<typename T>
    WriterPlaceholder<T> writePlaceholder() {
        off_t position = ftello(out);
        write((T)0);
        return WriterPlaceholder<T>(*this, position);
    }
private:
    void write(uint16_t v) { writeUInt16(v); };
    void write(uint32_t v) { writeEmojicodeChar(v); };
    void write(unsigned char v) { writeByte(v); };
    
    FILE *out;
};

template <typename T>
class WriterPlaceholder {
    friend Writer;
public:
    WriterPlaceholder(Writer &w, off_t position) : writer(w), position(position) {};
    /** Writes a coin with the given value */
    void write(T value) {
        off_t oldPosition = ftello(writer.out);
        fseek(writer.out, position, SEEK_SET);
        writer.write(value);
        fseek(writer.out, oldPosition, SEEK_SET);
    }
protected:
    Writer &writer;
    off_t position;
};

class WriterCoinsCountPlaceholder: private WriterPlaceholder<EmojicodeCoin> {
    friend Writer;
public:
    void write();
private:
    WriterCoinsCountPlaceholder(Writer &w, off_t position, uint32_t writtenCoins)
        : WriterPlaceholder(w, position), oWrittenCoins(writtenCoins) {};
    uint32_t oWrittenCoins;
};

#endif /* Writer_hpp */
