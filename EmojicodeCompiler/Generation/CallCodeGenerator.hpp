//
//  CallCodeGenerator.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef CallCodeGenerator_hpp
#define CallCodeGenerator_hpp

#include "../AST/ASTExpr.hpp"
#include "../AST/ASTInitialization.hpp"
#include "../Functions/Initializer.hpp"
#include "../Functions/CallType.h"
#include "../Types/Protocol.hpp"
#include "FnCodeGenerator.hpp"

namespace EmojicodeCompiler {

class CallCodeGenerator {
public:
    explicit CallCodeGenerator(FnCodeGenerator *fncg, CallType callType) : fncg_(fncg), callType_(callType) {}
    llvm::Value* generate(const ASTExpr &callee, const Type &calleeType, const ASTArguments &args,
                          const std::u32string &name);
protected:
    virtual Function* lookupFunction(const Type &type, const std::u32string &name) {
        return type.typeDefinition()->lookupMethod(name);
    }

    virtual llvm::Value* createCall(const std::vector<Value *> &args, const Type &type, const std::u32string &name);
    FnCodeGenerator* fncg() const { return fncg_; }
private:
    llvm::Value* createDynamicDispatch(Function *function, const std::vector<Value *> &args);
    FnCodeGenerator *fncg_;
    CallType callType_;
};

class TypeMethodCallCodeGenerator : public CallCodeGenerator {
    using CallCodeGenerator::CallCodeGenerator;
protected:
    Function* lookupFunction(const Type &type, const std::u32string &name) override {
        return type.typeDefinition()->lookupTypeMethod(name);
    }
};

class InitializationCallCodeGenerator : public CallCodeGenerator {
    using CallCodeGenerator::CallCodeGenerator;
protected:
    Function* lookupFunction(const Type &type, const std::u32string &name) override {
        return type.typeDefinition()->lookupInitializer(name);
    }
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

class SuperCallCodeGenerator : private CallCodeGenerator {
public:
    SuperCallCodeGenerator(FnCodeGenerator *fncg) : CallCodeGenerator(fncg, CallType::DynamicDispatch) {}
    llvm::Value* generate(const Type& superType, const ASTArguments &args, const std::u32string &name) {
//        auto argSize = generateArguments(args);
//        fncg()->wr().writeInstruction(INS_GET_CLASS_FROM_INDEX);
//        fncg()->wr().writeInstruction(superType.eclass()->index);
//        writeInstructions(argSize, superType, name);
        return nullptr;
    }
};

class SuperInitializerCallCodeGenerator : public SuperCallCodeGenerator {
    using SuperCallCodeGenerator::SuperCallCodeGenerator;
protected:
    Function* lookupFunction(const Type &type, const std::u32string &name) override {
        return type.typeDefinition()->lookupInitializer(name);
    }
};

}  // namespace EmojicodeCompiler

#endif /* CallCodeGenerator_hpp */
