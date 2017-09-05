//
//  CallCodeGenerator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "CallCodeGenerator.hpp"
#include "../AST/ASTExpr.hpp"

namespace EmojicodeCompiler {

llvm::Value* CallCodeGenerator::generate(const ASTExpr &callee, const Type &calleeType, const ASTArguments &args,
                                         const std::u32string &name) {
    std::vector<Value *> argsVector;
    if (callType_ != CallType::StaticContextfreeDispatch) {
        argsVector.emplace_back(callee.generate(fncg_));
    }
    for (auto arg : args.arguments()) {
        argsVector.emplace_back(arg->generate(fncg_));
    }
    return createCall(argsVector, calleeType, name);
}

llvm::Value* VTInitializationCallCodeGenerator::generate(const std::shared_ptr<ASTGetVariable> &dest,
                                                         const Type &type, const ASTArguments &args,
                                                         const std::u32string &name) {
    return CallCodeGenerator::generate(*dest, type, args, name);
}

}  // namespace EmojicodeCompiler
