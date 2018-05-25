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
#include <Functions/Function.hpp>

namespace llvm {
class Value;
}  // namespace llvm

namespace EmojicodeCompiler {

class FunctionCodeGenerator;
class Type;
class Function;
class ASTArguments;

class CallCodeGenerator {
public:
    CallCodeGenerator(FunctionCodeGenerator *fg, CallType callType) : fg_(fg), callType_(callType) {}
    llvm::Value* generate(llvm::Value *callee, const Type &type, const ASTArguments &astArgs,
                          const std::u32string &name);

    CallType callType() const { return callType_; }
protected:
    virtual Function *lookupFunction(const Type &type, const std::u32string &name, bool imperative);

    std::vector<llvm::Value *> createArgsVector(llvm::Value *callee, const ASTArguments &args) const;
    FunctionCodeGenerator* fg() const { return fg_; }
    llvm::Value *createDynamicProtocolDispatch(Function *function, std::vector<llvm::Value *> args,
                                               const std::vector<Type> &genericArgs,
                                               llvm::Value *conformancePtr);
private:
    llvm::Value *createDynamicDispatch(Function *function, const std::vector<llvm::Value *> &args,
                                       const std::vector<Type> &genericArgs);
    llvm::Value *dispatchFromVirtualTable(Function *function, llvm::Value *virtualTable,
                                              const std::vector<llvm::Value *> &args,
                                              const std::vector<Type> &genericArguments);
    FunctionCodeGenerator *fg_;
    CallType callType_;

    llvm::Value *getProtocolCallee(std::vector<llvm::Value *> &args, llvm::LoadInst *conformance) const;
};

class TypeMethodCallCodeGenerator : public CallCodeGenerator {
    using CallCodeGenerator::CallCodeGenerator;
protected:
    Function *lookupFunction(const Type &type, const std::u32string &name, bool imperative) override;
};

class InitializationCallCodeGenerator : public CallCodeGenerator {
    using CallCodeGenerator::CallCodeGenerator;
protected:
    Function *lookupFunction(const Type &type, const std::u32string &name, bool imperative) override;
};

class MultiprotocolCallCodeGenerator : protected CallCodeGenerator {
public:
    using CallCodeGenerator::CallCodeGenerator;
    llvm::Value* generate(llvm::Value *callee, const Type &calleeType, const ASTArguments &args,
                          const std::u32string &name, size_t multiprotocolN);
};

}  // namespace EmojicodeCompiler

#endif /* CallCodeGenerator_hpp */
