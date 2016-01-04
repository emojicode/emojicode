//
//  Writer.h
//  Emojicode
//
//  Created by Theo Weidmann on 01.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef __Emojicode__Writer__
#define __Emojicode__Writer__

#include "EmojicodeCompiler.h"
#include "Procedure.h"

uint32_t writtenCoins;

/**
 * The writer finally writes all types to the byte file.
 */

/** Must be used to write any uint16_t to the file */
void writeUInt16(uint16_t value, FILE *out);

/** Writes a coin with the given value */
void writeCoin(EmojicodeCoin value, FILE *out);

/** Writes a single unicode character */
void writeEmojicodeChar(EmojicodeChar c, FILE *out);

/** Must be used to write any double to the file. */
void writeDouble(double val, FILE *out);

/** Returns true if the procedure is native. */
bool writeProcedureHeading(Procedure *p, FILE *out, off_t *metaPosition);

/**
 * Writes a placeholder coin. To replace the placeholder use `writeCoinAtPlaceholder`
 */
off_t writePlaceholderCoin(FILE *out);

/** Replaces the placeholder written using `writePlaceholderCoin`*/
void writeCoinAtPlaceholder(off_t placeholderPosition, uint32_t value, FILE *out);

/**
 * Write the placeholder for a function block meta.
 */
off_t writeFunctionBlockMetaPlaceholder(FILE *out);

/**
 * Writes the function block meta.
 * @param placeholderPosition The value returned by `writeFunctionBlockMetaPlaceholder`.
 * @param writtenCoins The number of coins written.
 * @param variableCount The number of variables.
 */
void writeFunctionBlockMeta(off_t placeholderPosition, uint32_t writtenCoins, uint8_t variableCount, FILE *out);

#endif /* defined(__Emojicode__Writer__) */
