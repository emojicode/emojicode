//
//  Writer.h
//  Emojicode
//
//  Created by Theo Weidmann on 01.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef Writer_hpp
#define Writer_hpp

#include "../EmojicodeCompiler.hpp"

namespace EmojicodeCompiler {

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
    friend WriterPlaceholder<EmojicodeInstruction>;
    friend WriterPlaceholder<unsigned char>;
public:
    explicit Writer(const std::string &path) : path_(path) {};

    /** Must be used to write any uint16_t to the file */
    void writeUInt16(uint16_t value);

    /** Writes a coin with the given value */
    void writeInstruction(EmojicodeInstruction value);

    /** Writes a single unicode character */
    void writeEmojicodeChar(EmojicodeChar c);

    void writeByte(unsigned char);

    void writeBytes(const char *bytes, size_t count);

    void writeFunction(Function *function);

    /// Finishes the writing. The file is not ready to be used before this method was called.
    void finish();

    /**
     * Writes a placeholder coin. To replace the placeholder use `writeInstructionAtPlaceholder`
     */
    template<typename T>
    WriterPlaceholder<T> writePlaceholder() {
        auto i = data_.size();
        write((T)0);
        return WriterPlaceholder<T>(*this, i);
    }
private:
    void write(uint16_t v) { writeUInt16(v); };
    void write(uint32_t v) { writeEmojicodeChar(v); };
    void write(uint8_t v) { writeByte(v); };

    const std::string &path_;
    std::string data_;
};

template <typename T>
class WriterPlaceholder {
    friend Writer;
public:
    WriterPlaceholder(Writer &w, size_t index) : writer_(w), index_(index) {};
    /** Writes a coin with the given value */
    void write(T value) {
        if (std::is_same<T, uint8_t>::value) {
            writer_.data_[index_] = value;
        }
        else if (std::is_same<T, uint16_t>::value) {
            writer_.data_[index_] = static_cast<char>(value);
            writer_.data_[index_ + 1] = static_cast<char>(value >> 8);
        }
        else if (std::is_same<T, uint32_t>::value) {
            writer_.data_[index_] = static_cast<char>(value);
            writer_.data_[index_ + 1] = static_cast<char>(value >> 8);
            writer_.data_[index_ + 2] = static_cast<char>(value >> 16);
            writer_.data_[index_ + 3] = static_cast<char>(value >> 24);
        }
    }
protected:
    Writer &writer_;
    size_t index_;
};

class WriterCoinsCountPlaceholder: private WriterPlaceholder<EmojicodeInstruction> {
    friend Writer;
public:
    void write();
private:
    WriterCoinsCountPlaceholder(Writer &w, off_t position, uint32_t count)
        : WriterPlaceholder(w, position), oWrittenCoins(count) {};
    uint32_t oWrittenCoins;
};

}  // namespace EmojicodeCompiler

#endif /* Writer_hpp */
