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
    explicit CallCodeGenerator(FnCodeGenerator *fncg, CallType callType)
    : fncg_(fncg), callType_(callType) {}
    llvm::Value* generate(const ASTExpr &callee, const Type &calleeType, const ASTArguments &args,
                          const std::u32string &name);
protected:
    virtual Function* lookupFunction(const Type &type, const std::u32string &name) {
        return type.typeDefinition()->lookupMethod(name);
    }

    virtual llvm::Value* createCall(std::vector<Value *> args, const Type &type, const std::u32string &name) {
        return fncg_->builder().CreateCall(lookupFunction(type, name)->llvmFunction(), args);
    }

    FnCodeGenerator* fncg() { return fncg_; }
    EmojicodeInstruction generateArguments(const ASTArguments &args);
private:
    FnCodeGenerator *fncg_;
    CallType callType_;
};

class TypeMethodCallCodeGenerator : public CallCodeGenerator {
public:
    using CallCodeGenerator::CallCodeGenerator;
protected:
    Function* lookupFunction(const Type &type, const std::u32string &name) override {
        return type.typeDefinition()->lookupTypeMethod(name);
    }
};

class InitializationCallCodeGenerator : public CallCodeGenerator {
public:
    using CallCodeGenerator::CallCodeGenerator;
protected:
    Function* lookupFunction(const Type &type, const std::u32string &name) override {
        return type.typeDefinition()->lookupInitializer(name);
    }
};

class VTInitializationCallCodeGenerator : private InitializationCallCodeGenerator {
public:
    explicit VTInitializationCallCodeGenerator(FnCodeGenerator *fncg)
    : InitializationCallCodeGenerator(fncg, CallType::StaticDispatch) {}

    llvm::Value* generate(const std::shared_ptr<ASTGetVariable> &dest, const Type &type, const ASTArguments &args,
                          const std::u32string &name);
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
