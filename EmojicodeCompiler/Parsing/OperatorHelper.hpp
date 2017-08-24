//
//  OperatorHelper.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 01/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef OperatorHelper_hpp
#define OperatorHelper_hpp

#include "../EmojicodeCompiler.hpp"

namespace EmojicodeCompiler {

enum class OperatorType {
    MultiplicationOperator,
    DivisionOperator,
    RemainderOperator,
    MinusOperator,
    PlusOperator,
    ShiftLeftOperator,
    ShiftRightOperator,
    GreaterOrEqualOperator,
    GreaterOperator,
    LessOrEqualOperator,
    LessOperator,
    BitwiseAndOperator,
    BitwiseXorOperator,
    BitwiseOrOperator,
    LogicalAndOperator,
    LogicalOrOperator,
    EqualOperator,
    IdentityOperator,
    CallOperator,
};

/// @returns The precedence of the given operator. Operators with higher values are evaluated first.
int operatorPrecedence(OperatorType);
OperatorType operatorType(const std::u32string &);
std::u32string operatorName(OperatorType);
const int kPrefixPrecedence = 11;

} // namespace EmojicodeCompiler

#endif /* OperatorHelper_hpp */
