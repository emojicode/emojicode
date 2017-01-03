//
//  EmojicodeInstructions.h
//  Emojicode
//
//  Created by Theo Weidmann on 26/11/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef EmojicodeInstructions_h
#define EmojicodeInstructions_h

typedef enum EmojicodeInstruction {
    INS_DISPATCH_METHOD = 0x1,
    INS_DISPATCH_TYPE_METHOD = 0x2,
    INS_DISPATCH_PROTOCOL = 0x3,
    INS_NEW_OBJECT = 0x4,
    INS_DISPATCH_SUPER = 0x5,
    INS_CALL_CONTEXTED_FUNCTION = 0x6,
    INS_CALL_FUNCTION = 0x7,
    INS_PRODUCE_TO_AND_GET_VT_REFERENCE = 0x8,
    INS_INIT_VT = 0x9,
    INS_GET_VT_REFERENCE_STACK = 0xA,
    INS_GET_VT_REFERENCE_OBJECT = 0xB,
    INS_GET_VT_REFERENCE_VT = 0xC,
    INS_GET_CLASS_FROM_INSTANCE = 0xE,
    INS_GET_CLASS_FROM_INDEX = 0xF,
    INS_GET_STRING_POOL = 0x10,
    INS_GET_TRUE = 0x11,
    INS_GET_FALSE = 0x12,
    INS_GET_32_INTEGER = 0x13,
    INS_GET_64_INTEGER = 0x14,
    INS_GET_DOUBLE = 0x15,
    INS_GET_SYMBOL = 0x16,
    INS_GET_NOTHINGNESS = 0x17,
    INS_PRODUCE_WITH_STACK_DESTINATION = 0x18,
    INS_PRODUCE_WITH_OBJECT_DESTINATION = 0x19,
    INS_PRODUCE_WITH_VT_DESTINATION = 0x81,
    INS_INCREMENT = 0x1A,
    INS_DECREMENT = 0x1B,
    INS_COPY_SINGLE_STACK = 0x1C,
    INS_COPY_WITH_SIZE_STACK = 0x1D,
    INS_COPY_SINGLE_OBJECT = 0x1E,
    INS_COPY_WITH_SIZE_OBJECT = 0x1F,
    INS_COPY_SINGLE_VT = 0x80,
    INS_COPY_WITH_SIZE_VT = 0x82,
    INS_EQUAL_PRIMITIVE = 0x20,
    INS_SUBTRACT_INTEGER = 0x21,
    INS_ADD_INTEGER = 0x22,
    INS_MULTIPLY_INTEGER = 0x23,
    INS_DIVIDE_INTEGER = 0x24,
    INS_REMAINDER_INTEGER = 0x25,
    INS_INVERT_BOOLEAN = 0x26,
    INS_OR_BOOLEAN = 0x27,
    INS_AND_BOOLEAN = 0x28,
    INS_LESS_INTEGER = 0x29,
    INS_GREATER_INTEGER = 0x2A,
    INS_GREATER_OR_EQUAL_INTEGER = 0x2C,
    INS_LESS_OR_EQUAL_INTEGER = 0x2B,
    INS_SAME_OBJECT = 0x2D,
    INS_IS_NOTHINGNESS = 0x2E,
    INS_EQUAL_DOUBLE = 0x2F,
    INS_SUBTRACT_DOUBLE = 0x30,
    INS_ADD_DOUBLE = 0x31,
    INS_MULTIPLY_DOUBLE = 0x32,
    INS_DIVIDE_DOUBLE = 0x33,
    INS_LESS_DOUBLE = 0x34,
    INS_GREATER_DOUBLE = 0x35,
    INS_LESS_OR_EQUAL_DOUBLE = 0x36,
    INS_GREATER_OR_EQUAL_DOUBLE = 0x37,
    INS_REMAINDER_DOUBLE = 0x38,
    INS_INT_TO_DOUBLE = 0x39,
    INS_UNWRAP_OPTIONAL = 0x3A,
    INS_OPTIONAL_DISPATCH_METHOD = 0x3B,
    INS_GET_THIS = 0x3C,
    INS_SUPER_INITIALIZER = 0x3D,
    INS_CONDITIONAL_PRODUCE = 0x3E,
    INS_CAST_TO_CLASS = 0x40,
    INS_CAST_TO_PROTOCOL = 0x41,
    INS_SAFE_CAST_TO_CLASS = 0x42,
    INS_SAFE_CAST_TO_PROTOCOL = 0x43,
    INS_CAST_TO_BOOLEAN = 0x44,
    INS_CAST_TO_INTEGER = 0x45,
    INS_BINARY_AND_INTEGER = 0x5A,
    INS_BINARY_OR_INTEGER = 0x5B,
    INS_BINARY_XOR_INTEGER = 0x5C,
    INS_BINARY_NOT_INTEGER = 0x5D,
    INS_SHIFT_LEFT_INTEGER = 0x5E,
    INS_SHIFT_RIGHT_INTEGER = 0x5F,
    INS_REPEAT_WHILE = 0x61,
    INS_IF = 0x62,
    INS_FOREACH = 0x64,
    INS_RETURN = 0x60,
    INS_EXECUTE_CALLABLE = 0x70,
    INS_CLOSURE = 0x71,
    INS_CAPTURE_METHOD = 0x72,
    INS_CAPTURE_TYPE_METHOD = 0x73,
    INS_CAPTURE_CONTEXTED_FUNCTION = 0x74
} EmojicodeInstruction;

#ifdef DEBUG
void printInstructionDescription(EmojicodeInstruction ins) {
    switch (ins) {
        case INS_DISPATCH_METHOD: puts("INS_DISPATCH_METHOD"); return;
        case INS_DISPATCH_TYPE_METHOD: puts("INS_DISPATCH_TYPE_METHOD"); return;
        case INS_DISPATCH_PROTOCOL: puts("INS_DISPATCH_PROTOCOL"); return;
        case INS_NEW_OBJECT: puts("INS_NEW_OBJECT"); return;
        case INS_DISPATCH_SUPER: puts("INS_DISPATCH_SUPER"); return;
        case INS_CALL_CONTEXTED_FUNCTION: puts("INS_CALL_CONTEXTED_FUNCTION"); return;
        case INS_CALL_FUNCTION: puts("INS_CALL_FUNCTION"); return;
        case INS_PRODUCE_TO_AND_GET_VT_REFERENCE: puts("INS_PRODUCE_TO_AND_GET_VT_REFERENCE"); return;
        case INS_INIT_VT: puts("INS_INIT_VT"); return;
        case INS_GET_VT_REFERENCE_STACK: puts("INS_GET_VT_REFERENCE_STACK"); return;
        case INS_GET_VT_REFERENCE_OBJECT: puts("INS_GET_VT_REFERENCE_OBJECT"); return;
        case INS_GET_VT_REFERENCE_VT: puts("INS_GET_VT_REFERENCE_VT"); return;
        case INS_GET_CLASS_FROM_INSTANCE: puts("INS_GET_CLASS_FROM_INSTANCE"); return;
        case INS_GET_CLASS_FROM_INDEX: puts("INS_GET_CLASS_FROM_INDEX"); return;
        case INS_GET_STRING_POOL: puts("INS_GET_STRING_POOL"); return;
        case INS_GET_TRUE: puts("INS_GET_TRUE"); return;
        case INS_GET_FALSE: puts("INS_GET_FALSE"); return;
        case INS_GET_32_INTEGER: puts("INS_GET_32_INTEGER"); return;
        case INS_GET_64_INTEGER: puts("INS_GET_64_INTEGER"); return;
        case INS_GET_DOUBLE: puts("INS_GET_DOUBLE"); return;
        case INS_GET_SYMBOL: puts("INS_GET_SYMBOL"); return;
        case INS_GET_NOTHINGNESS: puts("INS_GET_NOTHINGNESS"); return;
        case INS_PRODUCE_WITH_STACK_DESTINATION: puts("INS_PRODUCE_WITH_STACK_DESTINATION"); return;
        case INS_PRODUCE_WITH_OBJECT_DESTINATION: puts("INS_PRODUCE_WITH_OBJECT_DESTINATION"); return;
        case INS_PRODUCE_WITH_VT_DESTINATION: puts("INS_PRODUCE_WITH_VT_DESTINATION"); return;
        case INS_INCREMENT: puts("INS_INCREMENT"); return;
        case INS_DECREMENT: puts("INS_DECREMENT"); return;
        case INS_COPY_SINGLE_STACK: puts("INS_COPY_SINGLE_STACK"); return;
        case INS_COPY_WITH_SIZE_STACK: puts("INS_COPY_WITH_SIZE_STACK"); return;
        case INS_COPY_SINGLE_OBJECT: puts("INS_COPY_SINGLE_OBJECT"); return;
        case INS_COPY_WITH_SIZE_OBJECT: puts("INS_COPY_WITH_SIZE_OBJECT"); return;
        case INS_COPY_SINGLE_VT: puts("INS_COPY_SINGLE_VT"); return;
        case INS_COPY_WITH_SIZE_VT: puts("INS_COPY_WITH_SIZE_VT"); return;
        case INS_EQUAL_PRIMITIVE: puts("INS_EQUAL_PRIMITIVE"); return;
        case INS_SUBTRACT_INTEGER: puts("INS_SUBTRACT_INTEGER"); return;
        case INS_ADD_INTEGER: puts("INS_ADD_INTEGER"); return;
        case INS_MULTIPLY_INTEGER: puts("INS_MULTIPLY_INTEGER"); return;
        case INS_DIVIDE_INTEGER: puts("INS_DIVIDE_INTEGER"); return;
        case INS_REMAINDER_INTEGER: puts("INS_REMAINDER_INTEGER"); return;
        case INS_INVERT_BOOLEAN: puts("INS_INVERT_BOOLEAN"); return;
        case INS_OR_BOOLEAN: puts("INS_OR_BOOLEAN"); return;
        case INS_AND_BOOLEAN: puts("INS_AND_BOOLEAN"); return;
        case INS_LESS_INTEGER: puts("INS_LESS_INTEGER"); return;
        case INS_GREATER_INTEGER: puts("INS_GREATER_INTEGER"); return;
        case INS_GREATER_OR_EQUAL_INTEGER: puts("INS_GREATER_OR_EQUAL_INTEGER"); return;
        case INS_LESS_OR_EQUAL_INTEGER: puts("INS_LESS_OR_EQUAL_INTEGER"); return;
        case INS_SAME_OBJECT: puts("INS_SAME_OBJECT"); return;
        case INS_IS_NOTHINGNESS: puts("INS_IS_NOTHINGNESS"); return;
        case INS_EQUAL_DOUBLE: puts("INS_EQUAL_DOUBLE"); return;
        case INS_SUBTRACT_DOUBLE: puts("INS_SUBTRACT_DOUBLE"); return;
        case INS_ADD_DOUBLE: puts("INS_ADD_DOUBLE"); return;
        case INS_MULTIPLY_DOUBLE: puts("INS_MULTIPLY_DOUBLE"); return;
        case INS_DIVIDE_DOUBLE: puts("INS_DIVIDE_DOUBLE"); return;
        case INS_LESS_DOUBLE: puts("INS_LESS_DOUBLE"); return;
        case INS_GREATER_DOUBLE: puts("INS_GREATER_DOUBLE"); return;
        case INS_LESS_OR_EQUAL_DOUBLE: puts("INS_LESS_OR_EQUAL_DOUBLE"); return;
        case INS_GREATER_OR_EQUAL_DOUBLE: puts("INS_GREATER_OR_EQUAL_DOUBLE"); return;
        case INS_REMAINDER_DOUBLE: puts("INS_REMAINDER_DOUBLE"); return;
        case INS_INT_TO_DOUBLE: puts("INS_INT_TO_DOUBLE"); return;
        case INS_UNWRAP_OPTIONAL: puts("INS_UNWRAP_OPTIONAL"); return;
        case INS_OPTIONAL_DISPATCH_METHOD: puts("INS_OPTIONAL_DISPATCH_METHOD"); return;
        case INS_GET_THIS: puts("INS_GET_THIS"); return;
        case INS_SUPER_INITIALIZER: puts("INS_SUPER_INITIALIZER"); return;
        case INS_CONDITIONAL_PRODUCE: puts("INS_CONDITIONAL_PRODUCE"); return;
        case INS_CAST_TO_CLASS: puts("INS_CAST_TO_CLASS"); return;
        case INS_CAST_TO_PROTOCOL: puts("INS_CAST_TO_PROTOCOL"); return;
        case INS_SAFE_CAST_TO_CLASS: puts("INS_SAFE_CAST_TO_CLASS"); return;
        case INS_SAFE_CAST_TO_PROTOCOL: puts("INS_SAFE_CAST_TO_PROTOCOL"); return;
        case INS_CAST_TO_BOOLEAN: puts("INS_CAST_TO_BOOLEAN"); return;
        case INS_CAST_TO_INTEGER: puts("INS_CAST_TO_INTEGER"); return;
        case INS_BINARY_AND_INTEGER: puts("INS_BINARY_AND_INTEGER"); return;
        case INS_BINARY_OR_INTEGER: puts("INS_BINARY_OR_INTEGER"); return;
        case INS_BINARY_XOR_INTEGER: puts("INS_BINARY_XOR_INTEGER"); return;
        case INS_BINARY_NOT_INTEGER: puts("INS_BINARY_NOT_INTEGER"); return;
        case INS_SHIFT_LEFT_INTEGER: puts("INS_SHIFT_LEFT_INTEGER"); return;
        case INS_SHIFT_RIGHT_INTEGER: puts("INS_SHIFT_RIGHT_INTEGER"); return;
        case INS_REPEAT_WHILE: puts("INS_REPEAT_WHILE"); return;
        case INS_IF: puts("INS_IF"); return;
        case INS_FOREACH: puts("INS_FOREACH"); return;
        case INS_RETURN: puts("INS_RETURN"); return;
        case INS_EXECUTE_CALLABLE: puts("INS_EXECUTE_CALLABLE"); return;
        case INS_CLOSURE: puts("INS_CLOSURE"); return;
        case INS_CAPTURE_METHOD: puts("INS_CAPTURE_METHOD"); return;
        case INS_CAPTURE_TYPE_METHOD: puts("INS_CAPTURE_TYPE_METHOD"); return;
        case INS_CAPTURE_CONTEXTED_FUNCTION: puts("INS_CAPTURE_CONTEXTED_FUNCTION"); return;
    }
    printf("UNKNOWN: 0x%X\n", ins);
}
#endif

#endif /* EmojicodeInstructions_h */
