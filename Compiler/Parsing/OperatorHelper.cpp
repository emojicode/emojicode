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
                return OperatorType::LessOrEqualOperator;
            case E_RIGHT_POINTING_TRIANGLE:
                return OperatorType::GreaterOrEqualOperator;
        }
    }
    switch (value.front()) {
        case E_HEAVY_PLUS_SIGN:
            return OperatorType::PlusOperator;
        case E_HEAVY_MINUS_SIGN:
            return OperatorType::MinusOperator;
        case E_HEAVY_DIVISION_SIGN:
            return OperatorType::DivisionOperator;
        case E_HEAVY_MULTIPLICATION_SIGN:
            return OperatorType::MultiplicationOperator;
        case E_OPEN_HANDS:
            return OperatorType::LogicalOrOperator;
        case E_HANDSHAKE:
            return OperatorType::LogicalAndOperator;
        case E_LEFT_POINTING_TRIANGLE:
            return OperatorType::LessOperator;
        case E_RIGHT_POINTING_TRIANGLE:
            return OperatorType::GreaterOperator;
        case E_HEAVY_LARGE_CIRCLE:
            return OperatorType::BitwiseAndOperator;
        case E_ANGER_SYMBOL:
            return OperatorType::BitwiseOrOperator;
        case E_CROSS_MARK:
            return OperatorType::BitwiseXorOperator;
        case E_LEFT_POINTING_BACKHAND_INDEX:
            return OperatorType::ShiftLeftOperator;
        case E_RIGHT_POINTING_BACKHAND_INDEX:
            return OperatorType::ShiftRightOperator;
        case E_PUT_LITTER_IN_ITS_SPACE:
            return OperatorType::RemainderOperator;
        case E_HANDS_RAISED_IN_CELEBRATION:
            return OperatorType::EqualOperator;
        case E_FACE_WITH_STUCK_OUT_TONGUE_AND_WINKING_EYE:
            return OperatorType::IdentityOperator;
    }
    return OperatorType::Invalid;
}

std::u32string operatorName(OperatorType type) {
    switch (type) {
        case OperatorType::PlusOperator:
            return std::u32string(1, E_HEAVY_PLUS_SIGN);
        case OperatorType::MinusOperator:
            return std::u32string(1, E_HEAVY_MINUS_SIGN);
        case OperatorType::DivisionOperator:
            return std::u32string(1, E_HEAVY_DIVISION_SIGN);
        case OperatorType::MultiplicationOperator:
            return std::u32string(1, E_HEAVY_MULTIPLICATION_SIGN);
        case OperatorType::LogicalOrOperator:
            return std::u32string(1, E_OPEN_HANDS);
        case OperatorType::LogicalAndOperator:
            return std::u32string(1, E_HANDSHAKE);
        case OperatorType::LessOperator:
            return std::u32string(1, E_LEFT_POINTING_TRIANGLE);
        case OperatorType::GreaterOperator:
            return std::u32string(1, E_RIGHT_POINTING_TRIANGLE);
        case OperatorType::LessOrEqualOperator:
            return { E_LEFT_POINTING_TRIANGLE, E_HANDS_RAISED_IN_CELEBRATION };
        case OperatorType::GreaterOrEqualOperator:
            return { E_RIGHT_POINTING_TRIANGLE, E_HANDS_RAISED_IN_CELEBRATION };
        case OperatorType::BitwiseAndOperator:
            return std::u32string(1, E_HEAVY_LARGE_CIRCLE);
        case OperatorType::BitwiseOrOperator:
            return std::u32string(1, E_ANGER_SYMBOL);
        case OperatorType::BitwiseXorOperator:
            return std::u32string(1, E_CROSS_MARK);
        case OperatorType::ShiftLeftOperator:
            return std::u32string(1, E_LEFT_POINTING_BACKHAND_INDEX);
        case OperatorType::ShiftRightOperator:
            return std::u32string(1, E_RIGHT_POINTING_BACKHAND_INDEX);
        case OperatorType::RemainderOperator:
            return std::u32string(1, E_PUT_LITTER_IN_ITS_SPACE);
        case OperatorType::EqualOperator:
            return std::u32string(1, E_HANDS_RAISED_IN_CELEBRATION);
        case OperatorType::IdentityOperator:
            return std::u32string(1, E_FACE_WITH_STUCK_OUT_TONGUE_AND_WINKING_EYE);
        case OperatorType::Invalid:
            throw std::invalid_argument("Invalid operator");
    }
}

int operatorPrecedence(OperatorType type) {
    switch (type) {
        case OperatorType::MultiplicationOperator:
        case OperatorType::DivisionOperator:
        case OperatorType::RemainderOperator:
            return 10;
        case OperatorType::MinusOperator:
        case OperatorType::PlusOperator:
            return 9;
        case OperatorType::ShiftLeftOperator:
        case OperatorType::ShiftRightOperator:
            return 8;
        case OperatorType::GreaterOrEqualOperator:
        case OperatorType::GreaterOperator:
        case OperatorType::LessOrEqualOperator:
        case OperatorType::LessOperator:
            return 7;
        case OperatorType::EqualOperator:
        case OperatorType::IdentityOperator:
            return 6;
        case OperatorType::BitwiseAndOperator:
            return 5;
        case OperatorType::BitwiseXorOperator:
            return 4;
        case OperatorType::BitwiseOrOperator:
            return 3;
        case OperatorType::LogicalAndOperator:
            return 2;
        case OperatorType::LogicalOrOperator:
            return 1;
        case OperatorType::Invalid:
            throw std::invalid_argument("Invalid operator");
    }
}

}  // namespace EmojicodeCompiler

