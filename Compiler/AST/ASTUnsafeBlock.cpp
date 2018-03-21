//
// Created by Theo Weidmann on 21.03.18.
//

#include "ASTUnsafeBlock.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Compiler.hpp"
#include "CompilerError.hpp"

namespace EmojicodeCompiler {

void ASTUnsafeBlock::analyse(FunctionAnalyser *analyser) {
    if (analyser->isInUnsafeBlock()) {
        analyser->compiler()->error(CompilerError(position(), "Already in a ☣️ block."));
        block_.analyse(analyser);
        return;
    }

    analyser->setInUnsafeBlock(true);
    block_.analyse(analyser);
    analyser->setInUnsafeBlock(false);
}

}  // namespace EmojicodeCompiler