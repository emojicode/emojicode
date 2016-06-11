//
//  Writer.c
//  Emojicode
//
//  Created by Theo Weidmann on 01.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include <cmath>
#include "Writer.hpp"
#include "CompilerErrorException.hpp"

void Writer::writeUInt16(uint16_t value) {
    fputc(value, out);
    fputc(value >> 8, out);
}

void Writer::writeEmojicodeChar(EmojicodeChar c) {
    fputc(c, out);
    fputc(c >> 8, out);
    fputc(c >> 16, out);
    fputc(c >> 24, out);
}

void Writer::writeCoin(EmojicodeCoin value, SourcePosition p) {
    fputc(value, out);
    fputc(value >> 8, out);
    fputc(value >> 16, out);
    fputc(value >> 24, out);
    
    if (++writtenCoins == 4294967295) {
        throw CompilerErrorException(p, "You exceeded the limit of 4294967295 allowed instructions in a function.");
    }
}

void Writer::writeByte(unsigned char c) {
    fputc(c, out);
}

void Writer::writeBytes(const char *bytes, size_t count) {
    fwrite(bytes, sizeof(char), count, out);
}

void Writer::writeDoubleCoin(double val, SourcePosition p) {
    int exp = 0;
    double norm = std::frexp(val, &exp);
    int_least64_t scale = norm*PORTABLE_INTLEAST64_MAX;
    
    writeCoin(scale >> 32, p);
    writeCoin((EmojicodeCoin)scale, p);
    writeCoin(exp, p);
}

WriterPlaceholder<EmojicodeCoin> Writer::writeCoinPlaceholder(SourcePosition p) {
    off_t position = ftello(out);
    writeCoin(0, p);
    return WriterPlaceholder<EmojicodeCoin>(*this, position);
}

WriterCoinsCountPlaceholder Writer::writeCoinsCountPlaceholderCoin(SourcePosition p) {
    off_t position = ftello(out);
    writeCoin(0, p);
    return WriterCoinsCountPlaceholder(*this, position, writtenCoins);
}

void WriterCoinsCountPlaceholder::write() {
    WriterPlaceholder<EmojicodeCoin>::write(writer.writtenCoins - oWrittenCoins);
}
