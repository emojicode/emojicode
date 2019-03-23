//
//  OperatorHelper.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 01/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef OperatorHelper_hpp
#define OperatorHelper_hpp

#include <string>

namespace EmojicodeCompiler {

enum class OperatorType {
    Invalid,
    Multiplication,
    Division,
    Remainder,
    Minus,
    Plus,
    ShiftLeft,
    ShiftRight,
    GreaterOrEqual,
    Greater,
    LessOrEqual,
    Less,
    BitwiseAnd,
    BitwiseXor,
    BitwiseOr,
    LogicalAnd,
    LogicalOr,
    Equal,
    Identity,
};

/// @returns The precedence of the given operator. Operators with higher values are evaluated first.
int operatorPrecedence(OperatorType);
OperatorType operatorType(const std::u32string &);
std::u32string operatorName(OperatorType);
const int kPrefixPrecedence = 11;

/// @returns True if a type can provide a custom definition of this operator.
bool canOperatorBeDefined(const std::u32string &oper);

} // namespace EmojicodeCompiler

#endif /* OperatorHelper_hpp */
