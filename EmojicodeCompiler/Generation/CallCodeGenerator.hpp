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
#include "../Types/Protocol.hpp"
#include "FnCodeGenerator.hpp"

namespace EmojicodeCompiler {

class CallCodeGenerator {
public:
    explicit CallCodeGenerator(FnCodeGenerator *fncg, EmojicodeInstruction instruction)
    : fncg_(fncg), instruction_(instruction) {}
    void generate(const ASTExpr &callee, const ASTArguments &args, const std::u32string &name) {
        auto argSize = generateArguments(args);
        callee.generate(fncg_);
        writeInstructions(argSize, callee.expressionType(), name);
    }
protected:
    virtual Function* lookupFunction(const Type &type, const std::u32string &name) {
        return type.typeDefinition()->lookupMethod(name);
    }

    virtual void writeInstructions(EmojicodeInstruction argSize, const Type &type, const std::u32string &name) {
        fncg_->wr().writeInstruction(instruction_);
        if (instruction_ == INS_DISPATCH_PROTOCOL) {
            fncg()->wr().writeInstruction(type.protocol()->index);
        }
        fncg_->wr().writeInstruction(lookupFunction(type, name)->vtiForUse());
        fncg_->wr().writeInstruction(argSize);
    }

    FnCodeGenerator* fncg() { return fncg_; }
    EmojicodeInstruction generateArguments(const ASTArguments &args);
private:
    FnCodeGenerator *fncg_;
    EmojicodeInstruction instruction_;
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
    : InitializationCallCodeGenerator(fncg, INS_CALL_CONTEXTED_FUNCTION) {}

    void generate(const std::shared_ptr<ASTVTInitDest> &dest, const Type &type, const ASTArguments &args,
                  const std::u32string &name) {
        EmojicodeInstruction argSize;
        if (dest == nullptr) {
            fncg()->wr().writeInstruction(INS_PUSH_N);
            fncg()->wr().writeInstruction(type.size());
            argSize = generateArguments(args);
            fncg()->wr().writeInstruction(INS_PUSH_STACK_REFERENCE_N_BACK);
            fncg()->wr().writeInstruction(argSize + type.size());
        }
        else {
            argSize = generateArguments(args);
            dest->generate(fncg());
        }
        writeInstructions(argSize, type, name);
        if (dest != nullptr) {
            dest->initialize(fncg());
        }
    }

};

class CallableCallCodeGenerator : public CallCodeGenerator {
public:
    explicit CallableCallCodeGenerator(FnCodeGenerator *fncg) : CallCodeGenerator(fncg, 0) {}
protected:
    void writeInstructions(EmojicodeInstruction argSize, const Type &type, const std::u32string &name) override {
        fncg()->wr().writeInstruction(INS_EXECUTE_CALLABLE);
        fncg()->wr().writeInstruction(argSize);
    }
};

class SuperCallCodeGenerator : private CallCodeGenerator {
public:
    using CallCodeGenerator::CallCodeGenerator;
    void generate(const Type& superType, const ASTArguments &args, const std::u32string &name) {
        auto argSize = generateArguments(args);
        fncg()->wr().writeInstruction(INS_GET_CLASS_FROM_INDEX);
        fncg()->wr().writeInstruction(superType.eclass()->index);
        writeInstructions(argSize, superType, name);
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
