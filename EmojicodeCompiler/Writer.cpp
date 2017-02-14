//
//  Writer.c
//  Emojicode
//
//  Created by Theo Weidmann on 01.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Writer.hpp"
#include <iostream>
#include "CompilerError.hpp"
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

void Writer::writeInstruction(EmojicodeInstruction value) {
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
    writeUInt16(function->getVti());
    writeByte(static_cast<uint8_t>(function->arguments.size()));

    writeUInt16(function->objectVariableInformation().size());
    for (auto info : function->objectVariableInformation()) {
        writeUInt16(info.index);
        writeUInt16(info.conditionIndex);
        writeUInt16(static_cast<uint16_t>(info.type));
        writeInstruction(info.from);
        writeInstruction(info.to);
    }

    writeUInt16(function->fullSize());

    if (function->isNative()) {
        writeUInt16(function->linkingTabelIndex());
        return;
    }
    writeUInt16(0);

    writeInstruction(static_cast<EmojicodeInstruction>(function->writer_.writtenInstructions()));

    for (auto coin : function->writer_.instructions_) {
        writeInstruction(coin);
    }
}
