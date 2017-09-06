//
//  CallCodeGenerator.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef CallCodeGenerator_hpp
#define CallCodeGenerator_hpp

#include "../Functions/CallType.h"
#include <string>

namespace llvm {
class Value;
}  // namespace llvm

namespace EmojicodeCompiler {

class FnCodeGenerator;
class Type;
class Function;
class ASTArguments;

class CallCodeGenerator {
public:
    CallCodeGenerator(FnCodeGenerator *fncg, CallType callType) : fncg_(fncg), callType_(callType) {}
    llvm::Value* generate(llvm::Value *callee, const Type &calleeType, const ASTArguments &args,
                          const std::u32string &name);
protected:
    virtual Function* lookupFunction(const Type &type, const std::u32string &name);
    virtual llvm::Value* createCall(const std::vector<llvm::Value *> &args, const Type &type,
                                    const std::u32string &name);
    FnCodeGenerator* fncg() const { return fncg_; }
private:
    llvm::Value* createDynamicDispatch(Function *function, const std::vector<llvm::Value *> &args);
    FnCodeGenerator *fncg_;
    CallType callType_;
};

class TypeMethodCallCodeGenerator : public CallCodeGenerator {
    using CallCodeGenerator::CallCodeGenerator;
protected:
    Function* lookupFunction(const Type &type, const std::u32string &name) override;
};

class InitializationCallCodeGenerator : public CallCodeGenerator {
    using CallCodeGenerator::CallCodeGenerator;
protected:
    Function* lookupFunction(const Type &type, const std::u32string &name) override;
};

class CallableCallCodeGenerator : public CallCodeGenerator {
public:
    explicit CallableCallCodeGenerator(FnCodeGenerator *fncg) : CallCodeGenerator(fncg, CallType::None) {}
protected:
//    void writeInstructions(EmojicodeInstruction argSize, const Type &type, const std::u32string &name) override {
//        fncg()->wr().writeInstruction(INS_EXECUTE_CALLABLE);
//        fncg()->wr().writeInstruction(argSize);
//    }
};

}  // namespace EmojicodeCompiler

#endif /* CallCodeGenerator_hpp */
