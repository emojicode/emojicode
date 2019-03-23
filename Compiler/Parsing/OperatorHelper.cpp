//
//  OperatorHelper.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 01/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "OperatorHelper.hpp"
#include "Emojis.h"
#include <stdexcept>

namespace EmojicodeCompiler {

OperatorType operatorType(const std::u32string &value) {
    if (value.size() >= 2) {
        switch (value.front()) {
            case E_LEFT_POINTING_TRIANGLE:
                return OperatorType::LessOrEqual;
            case E_RIGHT_POINTING_TRIANGLE:
                return OperatorType::GreaterOrEqual;
        }
    }
    switch (value.front()) {
        case E_HEAVY_PLUS_SIGN:
            return OperatorType::Plus;
        case E_HEAVY_MINUS_SIGN:
            return OperatorType::Minus;
        case E_HEAVY_DIVISION_SIGN:
            return OperatorType::Division;
        case E_HEAVY_MULTIPLICATION_SIGN:
            return OperatorType::Multiplication;
        case E_OPEN_HANDS:
            return OperatorType::LogicalOr;
        case E_HANDSHAKE:
            return OperatorType::LogicalAnd;
        case E_LEFT_POINTING_TRIANGLE:
            return OperatorType::Less;
        case E_RIGHT_POINTING_TRIANGLE:
            return OperatorType::Greater;
        case E_HEAVY_LARGE_CIRCLE:
            return OperatorType::BitwiseAnd;
        case E_ANGER_SYMBOL:
            return OperatorType::BitwiseOr;
        case E_CROSS_MARK:
            return OperatorType::BitwiseXor;
        case E_LEFT_POINTING_BACKHAND_INDEX:
            return OperatorType::ShiftLeft;
        case E_RIGHT_POINTING_BACKHAND_INDEX:
            return OperatorType::ShiftRight;
        case E_PUT_LITTER_IN_ITS_SPACE:
            return OperatorType::Remainder;
        case E_HANDS_RAISED_IN_CELEBRATION:
            return OperatorType::Equal;
        case E_FACE_WITH_STUCK_OUT_TONGUE_AND_WINKING_EYE:
            return OperatorType::Identity;
    }
    return OperatorType::Invalid;
}

std::u32string operatorName(OperatorType type) {
    switch (type) {
        case OperatorType::Plus:
            return std::u32string(1, E_HEAVY_PLUS_SIGN);
        case OperatorType::Minus:
            return std::u32string(1, E_HEAVY_MINUS_SIGN);
        case OperatorType::Division:
            return std::u32string(1, E_HEAVY_DIVISION_SIGN);
        case OperatorType::Multiplication:
            return std::u32string(1, E_HEAVY_MULTIPLICATION_SIGN);
        case OperatorType::LogicalOr:
            return std::u32string(1, E_OPEN_HANDS);
        case OperatorType::LogicalAnd:
            return std::u32string(1, E_HANDSHAKE);
        case OperatorType::Less:
            return std::u32string(1, E_LEFT_POINTING_TRIANGLE);
        case OperatorType::Greater:
            return std::u32string(1, E_RIGHT_POINTING_TRIANGLE);
        case OperatorType::LessOrEqual:
            return { E_LEFT_POINTING_TRIANGLE, E_HANDS_RAISED_IN_CELEBRATION };
        case OperatorType::GreaterOrEqual:
            return { E_RIGHT_POINTING_TRIANGLE, E_HANDS_RAISED_IN_CELEBRATION };
        case OperatorType::BitwiseAnd:
            return std::u32string(1, E_HEAVY_LARGE_CIRCLE);
        case OperatorType::BitwiseOr:
            return std::u32string(1, E_ANGER_SYMBOL);
        case OperatorType::BitwiseXor:
            return std::u32string(1, E_CROSS_MARK);
        case OperatorType::ShiftLeft:
            return std::u32string(1, E_LEFT_POINTING_BACKHAND_INDEX);
        case OperatorType::ShiftRight:
            return std::u32string(1, E_RIGHT_POINTING_BACKHAND_INDEX);
        case OperatorType::Remainder:
            return std::u32string(1, E_PUT_LITTER_IN_ITS_SPACE);
        case OperatorType::Equal:
            return std::u32string(1, E_HANDS_RAISED_IN_CELEBRATION);
        case OperatorType::Identity:
            return std::u32string(1, E_FACE_WITH_STUCK_OUT_TONGUE_AND_WINKING_EYE);
        case OperatorType::Invalid:
            throw std::invalid_argument("Invalid operator");
    }
}

int operatorPrecedence(OperatorType type) {
    switch (type) {
        case OperatorType::Multiplication:
        case OperatorType::Division:
        case OperatorType::Remainder:
            return 10;
        case OperatorType::Minus:
        case OperatorType::Plus:
            return 9;
        case OperatorType::ShiftLeft:
        case OperatorType::ShiftRight:
            return 8;
        case OperatorType::GreaterOrEqual:
        case OperatorType::Greater:
        case OperatorType::LessOrEqual:
        case OperatorType::Less:
            return 7;
        case OperatorType::Equal:
        case OperatorType::Identity:
            return 6;
        case OperatorType::BitwiseAnd:
            return 5;
        case OperatorType::BitwiseXor:
            return 4;
        case OperatorType::BitwiseOr:
            return 3;
        case OperatorType::LogicalAnd:
            return 2;
        case OperatorType::LogicalOr:
            return 1;
        case OperatorType::Invalid:
            throw std::invalid_argument("Invalid operator");
    }
}

bool canOperatorBeDefined(const std::u32string &oper) {
    auto type = operatorType(oper);
    return type != OperatorType::LogicalAnd && type != OperatorType::LogicalOr && type != OperatorType::Identity;
}

}  // namespace EmojicodeCompiler

