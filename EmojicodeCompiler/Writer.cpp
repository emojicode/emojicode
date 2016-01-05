//
//  Writer.c
//  Emojicode
//
//  Created by Theo Weidmann on 01.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Writer.h"
#include <math.h>

uint32_t writtenCoins;

void writeUInt16(uint16_t value, FILE *out){
    fputc(value >> 8, out);
    fputc(value, out);
}

void writeEmojicodeChar(EmojicodeChar c, FILE *out){
    fputc(c >> 24, out);
    fputc(c >> 16, out);
    fputc(c >> 8, out);
    fputc(c, out);
}

void writeCoin(EmojicodeCoin value, FILE *out){
    fputc(value >> 24, out);
    fputc(value >> 16, out);
    fputc(value >> 8, out);
    fputc(value, out);
    
    if(++writtenCoins == 4294967295) {
        compilerError(NULL, "Congratulations, you exceeded the limit of 4294967295 allowed instructions in a method/initializer.");
    }
}

void writeDouble(double val, FILE *out) {
    int exp = 0;
    double norm = frexp(val, &exp);
    int_least64_t scale = norm*PORTABLE_INTLEAST64_MAX;
    
    writeCoin(scale >> 32, out);
    writeCoin((EmojicodeCoin)scale, out);
    writeCoin(exp, out);
}

off_t writePlaceholderCoin(FILE *out){
    off_t position = ftello(out);
    writeCoin(0, out);
    return position;
}

void writeCoinAtPlaceholder(off_t placeholderPosition, uint32_t coinsInBlock, FILE *out){
    off_t oldPosition = ftello(out);
    fseek(out, placeholderPosition, SEEK_SET);
    
    writtenCoins--;
    writeCoin(coinsInBlock, out);
    
    fseek(out, oldPosition, SEEK_SET);
}

off_t writeFunctionBlockMetaPlaceholder(FILE *out){
    off_t countPosition = ftello(out);

    fputc(0, out);
    writeEmojicodeChar(0, out);
    
    return countPosition;
}

void writeFunctionBlockMeta(off_t placeholderPosition, uint32_t writtenCoins, uint8_t variableCount, FILE *out){
    off_t oldPosition = ftello(out);
    fseek(out, placeholderPosition, SEEK_SET);
    
    fputc(variableCount, out);
    writeEmojicodeChar(writtenCoins, out);
    
    fseek(out, oldPosition, SEEK_SET);
}

bool writeProcedureHeading(Procedure *p, FILE *out, off_t *metaPosition){
    writeEmojicodeChar(p->name, out);
    writeUInt16(p->vti, out);
    fputc((uint8_t)p->arguments.size(), out);
    
    if(p->native){
        fputc(1, out);
        return true;
    }
    fputc(0, out);
    
    writtenCoins = 0;
    *metaPosition = writeFunctionBlockMetaPlaceholder(out);
    return false;
}

