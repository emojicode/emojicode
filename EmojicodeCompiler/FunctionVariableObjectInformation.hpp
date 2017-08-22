//
//  FunctionVariableObjectInformation.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 02/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef FunctionVariableObjectInformation_hpp
#define FunctionVariableObjectInformation_hpp

#include "Types/Type.hpp"

namespace EmojicodeCompiler {

struct FunctionObjectVariableInformation : public ObjectVariableInformation {
    FunctionObjectVariableInformation(int index, ObjectVariableType type, InstructionCount from, InstructionCount to)
    : ObjectVariableInformation(index, type), from(from), to(to) {}
    FunctionObjectVariableInformation(int index, int condition, ObjectVariableType type, InstructionCount from,
                                      InstructionCount to)
    : ObjectVariableInformation(index, condition, type), from(from), to(to) {}
    int from;
    int to;
};

}

#endif /* FunctionVariableObjectInformation_hpp */
