//
//  ClosureCodeGenerator.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 21/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ClosureCodeGenerator_hpp
#define ClosureCodeGenerator_hpp

#include "Scoping/CapturingSemanticScoper.hpp"
#include "FunctionCodeGenerator.hpp"

namespace EmojicodeCompiler {

class ClosureCodeGenerator : public FunctionCodeGenerator {
public:
    ClosureCodeGenerator(std::vector<VariableCapture> captures, Function *f, CodeGenerator *generator)
    : FunctionCodeGenerator(f, generator), captures_(std::move(captures)) {}

    llvm::Value* thisValue() const override { return thisValue_; }
private:
    void declareArguments(llvm::Function *llvmFunction) override;
    std::vector<VariableCapture> captures_;
    llvm::Value *thisValue_;
};

} // namespace EmojicodeCompiler

#endif /* ClosureCodeGenerator_hpp */
