//
//  Writer.c
//  Emojicode
//
//  Created by Theo Weidmann on 01.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Writer.hpp"
#include "CompilerErrorException.hpp"
#include "Function.hpp"
#include "CallableWriter.hpp"

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

void Writer::writeCoin(EmojicodeCoin value) {
    fputc(value, out);
    fputc(value >> 8, out);
    fputc(value >> 16, out);
    fputc(value >> 24, out);
}

void Writer::writeByte(unsigned char c) {
    fputc(c, out);
}

void Writer::writeBytes(const char *bytes, size_t count) {
    fwrite(bytes, sizeof(char), count, out);
}

void Writer::writeFunction(Function *function) {
    writeEmojicodeChar(function->name());
    writeUInt16(function->getVti());
    writeByte(static_cast<uint8_t>(function->arguments.size()));
    
    if (function->native) {
        writeByte(function->typeMethod() ? 2 : 1);
        return;
    }
    writeByte(0);
    
    writeByte(function->maxVariableCount());
    writeCoin(static_cast<EmojicodeCoin>(function->writer_.writtenCoins()));

    for (auto coin : function->writer_.coins_) {
        writeCoin(coin);
    }
}
