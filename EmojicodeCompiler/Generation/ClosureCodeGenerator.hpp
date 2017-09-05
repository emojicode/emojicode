//
//  ClosureCodeGenerator.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 21/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ClosureCodeGenerator_hpp
#define ClosureCodeGenerator_hpp

#include "../Scoping/CapturingSemanticScoper.hpp"
#include "FnCodeGenerator.hpp"

namespace EmojicodeCompiler {

class ClosureCodeGenerator : public FnCodeGenerator {
public:
    ClosureCodeGenerator(std::vector<VariableCapture> captures, Function *f, CodeGenerator *generator)
    : FnCodeGenerator(f, generator), captures_(std::move(captures)) {}
    size_t captureSize() const { return captureSize_; }
    size_t captureDestIndex() const { return captureDestIndex_; }
private:
    void declareArguments(llvm::Function *function) override;
    size_t captureSize_ = 0;
    std::vector<VariableCapture> captures_;
    size_t captureDestIndex_ = 0;
};

} // namespace EmojicodeCompiler

#endif /* ClosureCodeGenerator_hpp */
