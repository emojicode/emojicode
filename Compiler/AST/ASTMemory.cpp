//
//  ASTMemory.cpp
//  runtime
//
//  Created by Theo Weidmann on 17.07.18.
//

#include "ASTMemory.hpp"
#include "Generation/CallCodeGenerator.hpp"
#include "Generation/FunctionCodeGenerator.hpp"

namespace EmojicodeCompiler {

void ASTRelease::generate(FunctionCodeGenerator *fg) const {
    release(fg);
}

void ASTRetain::generate(FunctionCodeGenerator *fg) const {
    fg->retain(managementValue(fg), variableType());
}

void ASTSuperDeinitializer::generate(FunctionCodeGenerator *fg) const {
    CallCodeGenerator(fg, CallType::StaticDispatch).generate(fg->thisValue(), Type::noReturn(),
                                                             ASTArguments(position(), true), deinit_);
}

} // namespace EmojicodeCompiler
