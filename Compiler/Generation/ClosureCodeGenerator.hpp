//
//  ClosureCodeGenerator.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 21/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ClosureCodeGenerator_hpp
#define ClosureCodeGenerator_hpp

#include "AST/ASTClosure.hpp"
#include "FunctionCodeGenerator.hpp"

namespace EmojicodeCompiler {

/// ClosureCodeGenerator is responsible for creating the IR for a closure.
/// In addition to the functionality inherited from FunctionCodeGenerator it manages the loading of captured variable
/// and of the captured context (this) if available.
class ClosureCodeGenerator : public FunctionCodeGenerator {
public:
    /// Constructor for a normal closure.
    ClosureCodeGenerator(const Capture &capture, Function *f, CodeGenerator *generator)
        : FunctionCodeGenerator(f, f->unspecificReification().function, generator), capture_(capture) {}

    /// Constructor for a callable thunk.
    ClosureCodeGenerator(Function *f, CodeGenerator *generator)
        : FunctionCodeGenerator(f, f->unspecificReification().function, generator), thunk_(true) {}

    llvm::Value* thisValue() const override {
        assert(capture_.capturesSelf() || thunk_);
        return thisValue_;
    }
    
private:
    void declareArguments(llvm::Function *llvmFunction) override;
    Capture capture_;
    llvm::Value *thisValue_ = nullptr;
    bool thunk_ = false;

    void loadCapturedVariables(Value *value);
};

} // namespace EmojicodeCompiler

#endif /* ClosureCodeGenerator_hpp */
