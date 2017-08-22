//
//  Writer.c
//  Emojicode
//
//  Created by Theo Weidmann on 01.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Writer.hpp"
#include "../../EmojicodeInstructions.h"
#include "../CompilerError.hpp"
#include "../Function.hpp"
#include "../FunctionType.hpp"
#include "FunctionWriter.hpp"
#include <fstream>
#include <iostream>

namespace EmojicodeCompiler {

void Writer::writeUInt16(uint16_t value) {
    data_.push_back(value);
    data_.push_back(value >> 8);
}

void Writer::writeEmojicodeChar(EmojicodeChar c) {
    data_.push_back(c);
    data_.push_back(c >> 8);
    data_.push_back(c >> 16);
    data_.push_back(c >> 24);
}

void Writer::writeInstruction(EmojicodeInstruction value) {
    data_.push_back(value);
    data_.push_back(value >> 8);
    data_.push_back(value >> 16);
    data_.push_back(value >> 24);
}

void Writer::writeByte(unsigned char c) {
    data_.push_back(c);
}

void Writer::writeBytes(const char *bytes, size_t count) {
    data_.append(bytes, count);
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

    writeByte(static_cast<unsigned char>(function->contextType()));
    writeUInt16(function->fullSize());


    writeUInt16(function->isNative() ? function->linkingTabelIndex() : 0);

    writeInstruction(static_cast<EmojicodeInstruction>(function->writer_.count()));

    for (auto coin : function->writer_.instructions_) {
        writeInstruction(coin);
    }
}

void Writer::finish() {
    auto out = std::ofstream(path_, std::ios::binary | std::ios::out | std::ios::trunc);
    if (out) {
        out.put(kByteCodeVersion);
        out.write(data_.data(), data_.size());
    }
    else {
        throw CompilerError(SourcePosition(0, 0, ""), "Couldn't write output file %s.", path_.c_str());
    }
}

}  // namespace EmojicodeCompiler
