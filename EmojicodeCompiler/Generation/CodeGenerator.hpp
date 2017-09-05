  //
//  CodeGenerator.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 19/09/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CodeGenerator_hpp
#define CodeGenerator_hpp

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include "../Types/Type.hpp"
#include <map>
#include <string>

namespace EmojicodeCompiler {

class Application;

/// Manages the generation of code
class CodeGenerator {
public:
    CodeGenerator(Application *app);
    llvm::LLVMContext& context() { return context_; }
    llvm::Module* module() { return module_.get(); }

    /// Returns the application for which this code generator was created.
    Application* app() { return application_; }

    void generate(const std::string &outPath);

    llvm::Type* llvmTypeForType(Type type);
    std::string mangleFunctionName(Function *function);
private:
    void mangleIdentifier(std::stringstream &stream, const std::u32string &string);
    std::string mangleTypeName(TypeDefinition *typeDef);

    llvm::Type* createLlvmTypeForTypeDefinition(const Type &type);

    Application *application_;
    llvm::LLVMContext context_;
    std::unique_ptr<llvm::Module> module_;
    std::map<Type, llvm::Type*> types_;
    std::unique_ptr<llvm::legacy::FunctionPassManager> passManager_;

    void setUpPassManager();
    void createLlvmFunction(Function *function);
};

}  // namespace EmojicodeCompiler

#endif /* CodeGenerator_hpp */
