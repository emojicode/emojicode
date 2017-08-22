//
//  EmojicodeInstructions.h
//  Emojicode
//
//  Created by Theo Weidmann on 26/11/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef EmojicodeInstructions_h
#define EmojicodeInstructions_h

/// A number identifying the set of byte code instructions and layout in use
const int kByteCodeVersion = 6;

enum Instructions {
    INS_DISPATCH_METHOD = 0x1,
    INS_DISPATCH_TYPE_METHOD = 0x2,
    INS_DISPATCH_PROTOCOL = 0x3,
    INS_DISPATCH_SUPER = 0x4,
    INS_CALL_CONTEXTED_FUNCTION = 0x5,
    INS_CALL_FUNCTION = 0x6,
    INS_SUPER_INITIALIZER = 0x8,
    INS_NEW_OBJECT = 0x9,

    INS_JUMP_FORWARD = 0x10,
    INS_JUMP_FORWARD_IF = 0x11,
    INS_JUMP_BACKWARD_IF = 0x12,
    INS_JUMP_FORWARD_IF_NOT = 0x13,
    INS_JUMP_BACKWARD_IF_NOT = 0x14,
    INS_RETURN = 0x16,
    INS_TRANSFER_CONTROL_TO_NATIVE = 0x18,

    INS_THIS = 0x19,
    INS_SAME_OBJECT = 0x1A,

    INS_COPY_TO_STACK = 0x21,
    INS_COPY_TO_INSTANCE_VARIABLE = 0x22,
    INS_COPY_VT_VARIABLE = 0x23,
    INS_COPY_TO_STACK_SIZE = 0xB1,
    INS_COPY_TO_INSTANCE_VARIABLE_SIZE = 0xB2,
    INS_COPY_VT_VARIABLE_SIZE = 0xB3,

    INS_PUSH_SINGLE_STACK = 0x26,
    INS_PUSH_WITH_SIZE_STACK = 0x27,
    INS_PUSH_SINGLE_OBJECT = 0x28,
    INS_PUSH_WITH_SIZE_OBJECT = 0x29,
    INS_PUSH_SINGLE_VT = 0x2A,
    INS_PUSH_WITH_SIZE_VT = 0x2B,
    INS_PUSH_VALUE_FROM_REFERENCE = 0x2C,

    INS_PUSH_N = 0x24,
    INS_POP = 0x97,
    INS_PUSH_STACK_REFERENCE_N_BACK = 0x96,

    INS_INCREMENT = 0x2D,
    INS_DECREMENT = 0x2E,

    INS_SIMPLE_OPTIONAL_PRODUCE = 0x30,
    INS_PUSH_ERROR = 0x25,
    INS_BOX_PRODUCE = 0x31,
    INS_UNBOX = 0x32,
    INS_BOX_TO_SIMPLE_OPTIONAL_PRODUCE = 0x33,
    INS_SIMPLE_OPTIONAL_TO_BOX = 0x34,
    INS_SIMPLE_OPTIONAL_TO_BOX_REMOTE = 0x35,
    INS_BOX_PRODUCE_REMOTE = 0x36,
    INS_UNBOX_REMOTE = 0x37,
    INS_BOX_TO_SIMPLE_OPTIONAL_PRODUCE_REMOTE = 0x38,

    INS_PUSH_VT_REFERENCE_STACK = 0x40,
    INS_PUSH_VT_REFERENCE_OBJECT = 0x41,
    INS_PUSH_VT_REFERENCE_VT = 0x42,
    INS_GET_CLASS_FROM_INSTANCE = 0x43,
    INS_GET_CLASS_FROM_INDEX = 0x44,
    INS_GET_STRING_POOL = 0x45,
    INS_GET_TRUE = 0x46,
    INS_GET_FALSE = 0x47,
    INS_GET_32_INTEGER = 0x48,
    INS_GET_64_INTEGER = 0x49,
    INS_GET_DOUBLE = 0x4A,
    INS_GET_SYMBOL = 0x4B,
    INS_GET_NOTHINGNESS = 0x4C,

    INS_EQUAL_PRIMITIVE = 0x50,
    INS_EQUAL_SYMBOL = 0x51,
    INS_SUBTRACT_INTEGER = 0x52,
    INS_ADD_INTEGER = 0x53,
    INS_MULTIPLY_INTEGER = 0x54,
    INS_DIVIDE_INTEGER = 0x55,
    INS_REMAINDER_INTEGER = 0x56,
    INS_BINARY_AND_INTEGER = 0x57,
    INS_BINARY_OR_INTEGER = 0x58,
    INS_BINARY_XOR_INTEGER = 0x59,
    INS_BINARY_NOT_INTEGER = 0x5A,
    INS_SHIFT_LEFT_INTEGER = 0x5B,
    INS_SHIFT_RIGHT_INTEGER = 0x5C,
    INS_GREATER_INTEGER = 0x5E,
    INS_GREATER_OR_EQUAL_INTEGER = 0x5F,

    INS_INVERT_BOOLEAN = 0x61,
    INS_OR_BOOLEAN = 0x62,
    INS_AND_BOOLEAN = 0x63,

    INS_EQUAL_DOUBLE = 0x64,
    INS_SUBTRACT_DOUBLE = 0x65,
    INS_ADD_DOUBLE = 0x66,
    INS_MULTIPLY_DOUBLE = 0x67,
    INS_DIVIDE_DOUBLE = 0x68,
    INS_GREATER_DOUBLE = 0x6A,
    INS_GREATER_OR_EQUAL_DOUBLE = 0x6C,
    INS_REMAINDER_DOUBLE = 0x6D,
    INS_INT_TO_DOUBLE = 0x6E,

    INS_UNWRAP_SIMPLE_OPTIONAL = 0x70,
    INS_UNWRAP_BOX_OPTIONAL = 0x71,
    INS_IS_NOTHINGNESS = 0x72,
    INS_ERROR_CHECK_SIMPLE_OPTIONAL = 0x73,
    INS_ERROR_CHECK_BOX_OPTIONAL = 0x74,
    INS_IS_ERROR = 0x75,

    // Know it’s an object instance and want to cast it to a subclass
    INS_DOWNCAST_TO_CLASS = 0x80,
    INS_CAST_TO_PROTOCOL = 0x81,
    INS_CAST_TO_CLASS = 0x82,
    INS_CAST_TO_VALUE_TYPE = 0x83,

    INS_EXECUTE_CALLABLE = 0x90,
    INS_CLOSURE = 0x91,
    INS_CAPTURE_METHOD = 0x92,
    INS_CAPTURE_TYPE_METHOD = 0x93,
    INS_CAPTURE_CONTEXTED_FUNCTION = 0x94,
    INS_CLOSURE_BOX = 0x95,
};

#include <stdio.h>

inline void pinsname(Instructions i) {switch(i) {
    case INS_DISPATCH_METHOD: printf("INS_DISPATCH_METHOD"); return;
    case INS_DISPATCH_TYPE_METHOD: printf("INS_DISPATCH_TYPE_METHOD"); return;
    case INS_DISPATCH_PROTOCOL: printf("INS_DISPATCH_PROTOCOL"); return;
    case INS_DISPATCH_SUPER: printf("INS_DISPATCH_SUPER"); return;
    case INS_CALL_CONTEXTED_FUNCTION: printf("INS_CALL_CONTEXTED_FUNCTION"); return;
    case INS_CALL_FUNCTION: printf("INS_CALL_FUNCTION"); return;
    case INS_SUPER_INITIALIZER: printf("INS_SUPER_INITIALIZER"); return;
    case INS_NEW_OBJECT: printf("INS_NEW_OBJECT"); return;
    case INS_JUMP_FORWARD: printf("INS_JUMP_FORWARD"); return;
    case INS_JUMP_FORWARD_IF: printf("INS_JUMP_FORWARD_IF"); return;
    case INS_JUMP_BACKWARD_IF: printf("INS_JUMP_BACKWARD_IF"); return;
    case INS_JUMP_FORWARD_IF_NOT: printf("INS_JUMP_FORWARD_IF_NOT"); return;
    case INS_JUMP_BACKWARD_IF_NOT: printf("INS_JUMP_BACKWARD_IF_NOT"); return;
    case INS_RETURN: printf("INS_RETURN"); return;
    case INS_TRANSFER_CONTROL_TO_NATIVE: printf("INS_TRANSFER_CONTROL_TO_NATIVE"); return;
    case INS_THIS: printf("INS_THIS"); return;
    case INS_SAME_OBJECT: printf("INS_SAME_OBJECT"); return;
    case INS_COPY_TO_STACK: printf("INS_COPY_TO_STACK"); return;
    case INS_COPY_TO_INSTANCE_VARIABLE: printf("INS_COPY_TO_INSTANCE_VARIABLE"); return;
    case INS_COPY_VT_VARIABLE: printf("INS_COPY_VT_VARIABLE"); return;
    case INS_COPY_TO_STACK_SIZE: printf("INS_COPY_TO_STACK_SIZE"); return;
    case INS_COPY_TO_INSTANCE_VARIABLE_SIZE: printf("INS_COPY_TO_INSTANCE_VARIABLE_SIZE"); return;
    case INS_COPY_VT_VARIABLE_SIZE: printf("INS_COPY_VT_VARIABLE_SIZE"); return;
    case INS_PUSH_SINGLE_STACK: printf("INS_PUSH_SINGLE_STACK"); return;
    case INS_PUSH_WITH_SIZE_STACK: printf("INS_PUSH_WITH_SIZE_STACK"); return;
    case INS_PUSH_SINGLE_OBJECT: printf("INS_PUSH_SINGLE_OBJECT"); return;
    case INS_PUSH_WITH_SIZE_OBJECT: printf("INS_PUSH_WITH_SIZE_OBJECT"); return;
    case INS_PUSH_SINGLE_VT: printf("INS_PUSH_SINGLE_VT"); return;
    case INS_PUSH_WITH_SIZE_VT: printf("INS_PUSH_WITH_SIZE_VT"); return;
    case INS_PUSH_VALUE_FROM_REFERENCE: printf("INS_PUSH_VALUE_FROM_REFERENCE"); return;
    case INS_PUSH_ERROR: printf("INS_PUSH_ERROR"); return;
    case INS_INCREMENT: printf("INS_INCREMENT"); return;
    case INS_DECREMENT: printf("INS_DECREMENT"); return;
    case INS_SIMPLE_OPTIONAL_PRODUCE: printf("INS_SIMPLE_OPTIONAL_PRODUCE"); return;
    case INS_BOX_PRODUCE: printf("INS_BOX_PRODUCE"); return;
    case INS_UNBOX: printf("INS_UNBOX"); return;
    case INS_BOX_TO_SIMPLE_OPTIONAL_PRODUCE: printf("INS_BOX_TO_SIMPLE_OPTIONAL_PRODUCE"); return;
    case INS_SIMPLE_OPTIONAL_TO_BOX: printf("INS_SIMPLE_OPTIONAL_TO_BOX"); return;
    case INS_SIMPLE_OPTIONAL_TO_BOX_REMOTE: printf("INS_SIMPLE_OPTIONAL_TO_BOX_REMOTE"); return;
    case INS_BOX_PRODUCE_REMOTE: printf("INS_BOX_PRODUCE_REMOTE"); return;
    case INS_UNBOX_REMOTE: printf("INS_UNBOX_REMOTE"); return;
    case INS_BOX_TO_SIMPLE_OPTIONAL_PRODUCE_REMOTE: printf("INS_BOX_TO_SIMPLE_OPTIONAL_PRODUCE_REMOTE"); return;
    case INS_PUSH_VT_REFERENCE_STACK: printf("INS_PUSH_VT_REFERENCE_STACK"); return;
    case INS_PUSH_VT_REFERENCE_OBJECT: printf("INS_PUSH_VT_REFERENCE_OBJECT"); return;
    case INS_PUSH_VT_REFERENCE_VT: printf("INS_PUSH_VT_REFERENCE_VT"); return;
    case INS_PUSH_N: printf("INS_PUSH_N"); return;
    case INS_GET_CLASS_FROM_INSTANCE: printf("INS_GET_CLASS_FROM_INSTANCE"); return;
    case INS_GET_CLASS_FROM_INDEX: printf("INS_GET_CLASS_FROM_INDEX"); return;
    case INS_GET_STRING_POOL: printf("INS_GET_STRING_POOL"); return;
    case INS_GET_TRUE: printf("INS_GET_TRUE"); return;
    case INS_GET_FALSE: printf("INS_GET_FALSE"); return;
    case INS_GET_32_INTEGER: printf("INS_GET_32_INTEGER"); return;
    case INS_GET_64_INTEGER: printf("INS_GET_64_INTEGER"); return;
    case INS_GET_DOUBLE: printf("INS_GET_DOUBLE"); return;
    case INS_GET_SYMBOL: printf("INS_GET_SYMBOL"); return;
    case INS_GET_NOTHINGNESS: printf("INS_GET_NOTHINGNESS"); return;
    case INS_EQUAL_PRIMITIVE: printf("INS_EQUAL_PRIMITIVE"); return;
    case INS_EQUAL_SYMBOL: printf("INS_EQUAL_SYMBOL"); return;
    case INS_SUBTRACT_INTEGER: printf("INS_SUBTRACT_INTEGER"); return;
    case INS_ADD_INTEGER: printf("INS_ADD_INTEGER"); return;
    case INS_MULTIPLY_INTEGER: printf("INS_MULTIPLY_INTEGER"); return;
    case INS_DIVIDE_INTEGER: printf("INS_DIVIDE_INTEGER"); return;
    case INS_REMAINDER_INTEGER: printf("INS_REMAINDER_INTEGER"); return;
    case INS_BINARY_AND_INTEGER: printf("INS_BINARY_AND_INTEGER"); return;
    case INS_BINARY_OR_INTEGER: printf("INS_BINARY_OR_INTEGER"); return;
    case INS_BINARY_XOR_INTEGER: printf("INS_BINARY_XOR_INTEGER"); return;
    case INS_BINARY_NOT_INTEGER: printf("INS_BINARY_NOT_INTEGER"); return;
    case INS_SHIFT_LEFT_INTEGER: printf("INS_SHIFT_LEFT_INTEGER"); return;
    case INS_SHIFT_RIGHT_INTEGER: printf("INS_SHIFT_RIGHT_INTEGER"); return;
    case INS_GREATER_INTEGER: printf("INS_GREATER_INTEGER"); return;
    case INS_GREATER_OR_EQUAL_INTEGER: printf("INS_GREATER_OR_EQUAL_INTEGER"); return;
    case INS_INVERT_BOOLEAN: printf("INS_INVERT_BOOLEAN"); return;
    case INS_OR_BOOLEAN: printf("INS_OR_BOOLEAN"); return;
    case INS_AND_BOOLEAN: printf("INS_AND_BOOLEAN"); return;
    case INS_EQUAL_DOUBLE: printf("INS_EQUAL_DOUBLE"); return;
    case INS_SUBTRACT_DOUBLE: printf("INS_SUBTRACT_DOUBLE"); return;
    case INS_ADD_DOUBLE: printf("INS_ADD_DOUBLE"); return;
    case INS_MULTIPLY_DOUBLE: printf("INS_MULTIPLY_DOUBLE"); return;
    case INS_DIVIDE_DOUBLE: printf("INS_DIVIDE_DOUBLE"); return;
    case INS_GREATER_DOUBLE: printf("INS_GREATER_DOUBLE"); return;
    case INS_GREATER_OR_EQUAL_DOUBLE: printf("INS_GREATER_OR_EQUAL_DOUBLE"); return;
    case INS_REMAINDER_DOUBLE: printf("INS_REMAINDER_DOUBLE"); return;
    case INS_INT_TO_DOUBLE: printf("INS_INT_TO_DOUBLE"); return;
    case INS_UNWRAP_SIMPLE_OPTIONAL: printf("INS_UNWRAP_SIMPLE_OPTIONAL"); return;
    case INS_UNWRAP_BOX_OPTIONAL: printf("INS_UNWRAP_BOX_OPTIONAL"); return;
    case INS_IS_NOTHINGNESS: printf("INS_IS_NOTHINGNESS"); return;
    case INS_ERROR_CHECK_SIMPLE_OPTIONAL: printf("INS_ERROR_CHECK_SIMPLE_OPTIONAL"); return;
    case INS_ERROR_CHECK_BOX_OPTIONAL: printf("INS_ERROR_CHECK_BOX_OPTIONAL"); return;
    case INS_IS_ERROR: printf("INS_IS_ERROR"); return;
    case INS_DOWNCAST_TO_CLASS: printf("INS_DOWNCAST_TO_CLASS"); return;
    case INS_CAST_TO_PROTOCOL: printf("INS_CAST_TO_PROTOCOL"); return;
    case INS_CAST_TO_CLASS: printf("INS_CAST_TO_CLASS"); return;
    case INS_CAST_TO_VALUE_TYPE: printf("INS_CAST_TO_VALUE_TYPE"); return;
    case INS_EXECUTE_CALLABLE: printf("INS_EXECUTE_CALLABLE"); return;
    case INS_CLOSURE: printf("INS_CLOSURE"); return;
    case INS_CAPTURE_METHOD: printf("INS_CAPTURE_METHOD"); return;
    case INS_CAPTURE_TYPE_METHOD: printf("INS_CAPTURE_TYPE_METHOD"); return;
    case INS_CAPTURE_CONTEXTED_FUNCTION: printf("INS_CAPTURE_CONTEXTED_FUNCTION"); return;
    case INS_CLOSURE_BOX: printf("INS_CLOSURE_BOX"); return;
    case INS_PUSH_STACK_REFERENCE_N_BACK: printf("INS_PUSH_STACK_REFERENCE_N_BACK"); return;
    case INS_POP: printf("INS_POP"); return;
}}

inline int inscount(Instructions i) {switch(i) {
    case INS_DISPATCH_METHOD: return 2;
    case INS_DISPATCH_TYPE_METHOD: return 2;
    case INS_DISPATCH_PROTOCOL: return 3;
    case INS_DISPATCH_SUPER: return 2;
    case INS_CALL_CONTEXTED_FUNCTION: return 2;
    case INS_CALL_FUNCTION: return 2;
    case INS_SUPER_INITIALIZER: return 2;
    case INS_NEW_OBJECT: return 2;
    case INS_JUMP_FORWARD: return 1;
    case INS_JUMP_FORWARD_IF: return 1;
    case INS_JUMP_BACKWARD_IF: return 1;
    case INS_JUMP_FORWARD_IF_NOT: return 1;
    case INS_JUMP_BACKWARD_IF_NOT: return 1;
    case INS_RETURN: return 0;
    case INS_TRANSFER_CONTROL_TO_NATIVE: return 0;
    case INS_THIS: return 0;
    case INS_SAME_OBJECT: return 0;
    case INS_COPY_TO_STACK: return 1;
    case INS_COPY_TO_INSTANCE_VARIABLE: return 1;
    case INS_COPY_VT_VARIABLE: return 1;
    case INS_COPY_TO_STACK_SIZE: return 2;
    case INS_COPY_TO_INSTANCE_VARIABLE_SIZE: return 2;
    case INS_COPY_VT_VARIABLE_SIZE: return 2;
    case INS_PUSH_SINGLE_STACK: return 1;
    case INS_PUSH_WITH_SIZE_STACK: return 2;
    case INS_PUSH_SINGLE_OBJECT: return 1;
    case INS_PUSH_WITH_SIZE_OBJECT: return 2;
    case INS_PUSH_SINGLE_VT: return 1;
    case INS_PUSH_WITH_SIZE_VT: return 2;
    case INS_PUSH_VALUE_FROM_REFERENCE: return 1;
    case INS_PUSH_ERROR: return 0;
    case INS_INCREMENT: return 0;
    case INS_DECREMENT: return 0;
    case INS_SIMPLE_OPTIONAL_PRODUCE: return 0;
    case INS_BOX_PRODUCE: return 1;
    case INS_UNBOX: return 1;
    case INS_BOX_TO_SIMPLE_OPTIONAL_PRODUCE: return 1;
    case INS_SIMPLE_OPTIONAL_TO_BOX: return 2;
    case INS_SIMPLE_OPTIONAL_TO_BOX_REMOTE: return 2;
    case INS_BOX_PRODUCE_REMOTE: return 2;
    case INS_UNBOX_REMOTE: return 1;
    case INS_BOX_TO_SIMPLE_OPTIONAL_PRODUCE_REMOTE: return 1;
    case INS_PUSH_VT_REFERENCE_STACK: return 1;
    case INS_PUSH_VT_REFERENCE_OBJECT: return 1;
    case INS_PUSH_VT_REFERENCE_VT: return 1;
    case INS_PUSH_N: return 1;
    case INS_GET_CLASS_FROM_INSTANCE: return 0;
    case INS_GET_CLASS_FROM_INDEX: return 1;
    case INS_GET_STRING_POOL: return 1;
    case INS_GET_TRUE: return 0;
    case INS_GET_FALSE: return 0;
    case INS_GET_32_INTEGER: return 1;
    case INS_GET_64_INTEGER: return 1;
    case INS_GET_DOUBLE: return 3;
    case INS_GET_SYMBOL: return 1;
    case INS_GET_NOTHINGNESS: return 0;
    case INS_EQUAL_PRIMITIVE: return 0;
    case INS_EQUAL_SYMBOL: return 0;
    case INS_SUBTRACT_INTEGER: return 0;
    case INS_ADD_INTEGER: return 0;
    case INS_MULTIPLY_INTEGER: return 0;
    case INS_DIVIDE_INTEGER: return 0;
    case INS_REMAINDER_INTEGER: return 0;
    case INS_BINARY_AND_INTEGER: return 0;
    case INS_BINARY_OR_INTEGER: return 0;
    case INS_BINARY_XOR_INTEGER: return 0;
    case INS_BINARY_NOT_INTEGER: return 0;
    case INS_SHIFT_LEFT_INTEGER: return 0;
    case INS_SHIFT_RIGHT_INTEGER: return 0;
    case INS_GREATER_INTEGER: return 0;
    case INS_GREATER_OR_EQUAL_INTEGER: return 0;
    case INS_INVERT_BOOLEAN: return 0;
    case INS_OR_BOOLEAN: return 0;
    case INS_AND_BOOLEAN: return 0;
    case INS_EQUAL_DOUBLE: return 0;
    case INS_SUBTRACT_DOUBLE: return 0;
    case INS_ADD_DOUBLE: return 0;
    case INS_MULTIPLY_DOUBLE:return 0;
    case INS_DIVIDE_DOUBLE: return 0;
    case INS_GREATER_DOUBLE: return 0;
    case INS_GREATER_OR_EQUAL_DOUBLE: return 0;
    case INS_REMAINDER_DOUBLE: return 0;
    case INS_INT_TO_DOUBLE: return 0;
    case INS_UNWRAP_SIMPLE_OPTIONAL: return 1;
    case INS_UNWRAP_BOX_OPTIONAL: return 0;
    case INS_IS_NOTHINGNESS: return 0;
    case INS_ERROR_CHECK_SIMPLE_OPTIONAL: return 0;
    case INS_ERROR_CHECK_BOX_OPTIONAL: return 0;
    case INS_IS_ERROR: return 0;
    case INS_DOWNCAST_TO_CLASS: return 1;
    case INS_CAST_TO_PROTOCOL: return 1;
    case INS_CAST_TO_CLASS: return 1;
    case INS_CAST_TO_VALUE_TYPE: return 1;
    case INS_EXECUTE_CALLABLE: return 1;
    case INS_CLOSURE: return 0;
    case INS_CAPTURE_METHOD: return 1;
    case INS_CAPTURE_TYPE_METHOD: return 1;
    case INS_CAPTURE_CONTEXTED_FUNCTION: return 1;
    case INS_CLOSURE_BOX:return 0;
    case INS_PUSH_STACK_REFERENCE_N_BACK: return 1;
    case INS_POP: return 0;
}}

#endif /* EmojicodeInstructions_h */
