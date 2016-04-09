//
//  Writer.c
//  Emojicode
//
//  Created by Theo Weidmann on 01.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Writer.hpp"
#include <cmath>

void Writer::writeUInt16(uint16_t value) {
    fputc(value >> 8, out);
    fputc(value, out);
}

void Writer::writeEmojicodeChar(EmojicodeChar c) {
    fputc(c >> 24, out);
    fputc(c >> 16, out);
    fputc(c >> 8, out);
    fputc(c, out);
}

void Writer::writeCoin(EmojicodeCoin value) {
    fputc(value >> 24, out);
    fputc(value >> 16, out);
    fputc(value >> 8, out);
    fputc(value, out);
    
    if (++writtenCoins == 4294967295) {
        compilerError(nullptr, "You exceeded the limit of 4294967295 allowed instructions in a procedure.");
    }
}

void Writer::writeByte(unsigned char c) {
    fputc(c, out);
}

void Writer::writeBytes(const char *bytes, size_t count) {
    fwrite(bytes, sizeof(char), count, out);
}

void Writer::writeDouble(double val) {
    int exp = 0;
    double norm = std::frexp(val, &exp);
    int_least64_t scale = norm*PORTABLE_INTLEAST64_MAX;
    
    writeCoin(scale >> 32);
    writeCoin((EmojicodeCoin)scale);
    writeCoin(exp);
}


WriterPlaceholder<EmojicodeCoin> Writer::writeCoinPlaceholder() {
    off_t position = ftello(out);
    writeCoin(0);
    return WriterPlaceholder<EmojicodeCoin>(*this, position);
}

WriterCoinsCountPlaceholder Writer::writeCoinsCountPlaceholderCoin() {
    off_t position = ftello(out);
    writeCoin(0);
    return WriterCoinsCountPlaceholder(*this, position, writtenCoins);
}

void WriterCoinsCountPlaceholder::write() {
    WriterPlaceholder<EmojicodeCoin>::write(writer.writtenCoins - oWrittenCoins);
}
