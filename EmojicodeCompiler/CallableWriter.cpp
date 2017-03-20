//
//  CallableWriter.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/09/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "CallableWriter.hpp"
#include "Token.hpp"
#include <cmath>

void CallableWriter::writeInstruction(EmojicodeInstruction value) {
    instructions_.push_back(value);
}

void CallableWriter::writeInstruction(std::initializer_list<EmojicodeInstruction> values) {
    for (auto value : values) {
        instructions_.push_back(value);
    }
}

void CallableWriter::writeDoubleCoin(double val) {
    int exp = 0;
    double norm = std::frexp(val, &exp);
    int_least64_t scale = norm*PORTABLE_INTLEAST64_MAX;

    writeInstruction(scale >> 32);
    writeInstruction(static_cast<EmojicodeInstruction>(scale));
    writeInstruction(exp);
}

CallableWriterPlaceholder CallableWriter::writeInstructionPlaceholder() {
    writeInstruction(0);
    return CallableWriterPlaceholder(this, instructions_.size() - 1);
}

CallableWriterCoinsCountPlaceholder CallableWriter::writeInstructionsCountPlaceholderCoin() {
    writeInstruction(0);
    return CallableWriterCoinsCountPlaceholder(this, instructions_.size() - 1, writtenInstructions());
}

CallableWriterInsertionPoint CallableWriter::getInsertionPoint() {
    return CallableWriterInsertionPoint(this, instructions_.size());
}

void CallableWriterPlaceholder::write(EmojicodeInstruction value) const {
    writer_->instructions_[index_] = value;
}

void CallableWriterCoinsCountPlaceholder::write() {
    CallableWriterPlaceholder::write(static_cast<EmojicodeInstruction>(writer_->writtenInstructions() - count_));
}

void CallableWriterInsertionPoint::insert(EmojicodeInstruction value) {
    writer_->instructions_.insert(writer_->instructions_.begin() + index_, value);
    index_++;
}

void CallableWriterInsertionPoint::insert(std::initializer_list<EmojicodeInstruction> values) {
    writer_->instructions_.insert(writer_->instructions_.begin() + index_, values);
    index_ += values.size();
}
