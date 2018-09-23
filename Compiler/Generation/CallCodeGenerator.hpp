//
//  CallCodeGenerator.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef CallCodeGenerator_hpp
#define CallCodeGenerator_hpp

#include "Functions/CallType.h"
#include <string>
#include <vector>
#include <llvm/IR/Instructions.h>

namespace llvm {
class Value;
}  // namespace llvm

namespace EmojicodeCompiler {

class FunctionCodeGenerator;
class Type;
class Function;
class ASTArguments;

/// This class is responsible for generating IR to dispatch a method, initializer, type method or protocol method.
///
/// It does all the heavy lifting concerning dispatch, dispatch tables etc.
///
/// It is also capable of generating code to find a protocol conformance if the callee of a dynamic protocol dispatch is
/// not boxed for the protocol in question.
class CallCodeGenerator {
public:
    CallCodeGenerator(FunctionCodeGenerator *fg, CallType callType) : fg_(fg), callType_(callType) {}
    llvm::Value* generate(llvm::Value *callee, const Type &type, const ASTArguments &astArgs,
                          Function *function);

protected:
    std::vector<llvm::Value *> createArgsVector(llvm::Value *callee, const ASTArguments &args) const;
    FunctionCodeGenerator* fg() const { return fg_; }
    llvm::Value *createDynamicProtocolDispatch(Function *function, std::vector<llvm::Value *> args,
                                               const std::vector<Type> &genericArgs,
                                               llvm::Value *conformance);
    llvm::Value* buildFindProtocolConformance(const std::vector<llvm::Value *> &args, const Type &protocol);
private:
    llvm::Value *createDynamicDispatch(Function *function, const std::vector<llvm::Value *> &args,
                                       const std::vector<Type> &genericArgs);
    llvm::Value *dispatchFromVirtualTable(Function *function, llvm::Value *virtualTable,
                                              const std::vector<llvm::Value *> &args,
                                              const std::vector<Type> &genericArguments);
    FunctionCodeGenerator *fg_;
    CallType callType_;

    llvm::Value *getProtocolCallee(std::vector<llvm::Value *> &args, llvm::Value *conformance) const;
};

class MultiprotocolCallCodeGenerator : protected CallCodeGenerator {
public:
    using CallCodeGenerator::CallCodeGenerator;
    llvm::Value* generate(llvm::Value *callee, const Type &calleeType, const ASTArguments &args,
                          Function *function, size_t multiprotocolN);
};

}  // namespace EmojicodeCompiler

#endif /* CallCodeGenerator_hpp */
