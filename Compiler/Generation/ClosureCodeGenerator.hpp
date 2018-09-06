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
#include "Scoping/CapturingSemanticScoper.hpp"

namespace EmojicodeCompiler {

class ClosureCodeGenerator : public FunctionCodeGenerator {
public:
    ClosureCodeGenerator(const Capture &capture, Function *f, CodeGenerator *generator)
    : FunctionCodeGenerator(f, f->unspecificReification().function, generator), capture_(capture) {}

    llvm::Value* thisValue() const override {
        assert(capture_.capturesSelf());
        return thisValue_;
    }
private:
    void declareArguments(llvm::Function *llvmFunction) override;
    const Capture &capture_;
    llvm::Value *thisValue_ = nullptr;

    void loadCapturedVariables(Value *value);
};

} // namespace EmojicodeCompiler

#endif /* ClosureCodeGenerator_hpp */
