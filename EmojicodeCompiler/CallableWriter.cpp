//
//  CallableWriter.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/09/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include <cmath>
#include "CallableWriter.hpp"
#include "Token.hpp"

void CallableWriter::writeCoin(EmojicodeCoin value, SourcePosition p) {
    coins_.push_back(value);
}

void CallableWriter::writeDoubleCoin(double val, SourcePosition p) {
    int exp = 0;
    double norm = std::frexp(val, &exp);
    int_least64_t scale = norm*PORTABLE_INTLEAST64_MAX;
    
    writeCoin(scale >> 32, p);
    writeCoin((EmojicodeCoin)scale, p);
    writeCoin(exp, p);
}

CallableWriterPlaceholder CallableWriter::writeCoinPlaceholder(SourcePosition p) {
    writeCoin(0, p);
    return CallableWriterPlaceholder(this, coins_.size() - 1);
}

CallableWriterCoinsCountPlaceholder CallableWriter::writeCoinsCountPlaceholderCoin(SourcePosition p) {
    writeCoin(0, p);
    return CallableWriterCoinsCountPlaceholder(this, coins_.size() - 1, writtenCoins());
}

void CallableWriterPlaceholder::write(EmojicodeCoin value) {
    writer_->coins_[index_] = value;
}

void CallableWriterCoinsCountPlaceholder::write() {
    CallableWriterPlaceholder::write(static_cast<EmojicodeCoin>(writer_->writtenCoins() - count_));
}
