//
//  ErrorSelfDestructing.cpp
//  runtime
//
//  Created by Theo Weidmann on 24.09.18.
//

#include "ErrorSelfDestructing.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Generation/Declarator.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "Scoping/SemanticScoper.hpp"
#include "Types/Class.hpp"

namespace EmojicodeCompiler {

void ErrorSelfDestructing::analyseInstanceVariables(FunctionAnalyser *analyser) {
    for (auto &var : analyser->scoper().instanceScope()->map()) {
        Variable &cv = var.second;
        if (cv.isInitialized() && cv.type().isManaged()) {
            release_.emplace_back(cv.id(), cv.type());
        }
    }
    class_ = dynamic_cast<Class*>(analyser->function()->owner());
    assert(class_ != nullptr);
}

void ErrorSelfDestructing::buildDestruct(FunctionCodeGenerator *fg) const {
    if (class_ != nullptr) {
        for (auto &release : release_) {
            fg->releaseByReference(fg->instanceVariablePointer(release.first), release.second);
        }
        auto clInf = fg->buildGetClassInfoFromObject(fg->thisValue());
        fg->createIf(fg->builder().CreateICmpEQ(clInf, class_->classInfo()), [&] {
            fg->builder().CreateCall(fg->generator()->declarator().releaseMemory(),
                                     fg->builder().CreateBitCast(fg->thisValue(),
                                                                 llvm::Type::getInt8PtrTy(fg->generator()->context())));
        });
    }
}

}  // namespace EmojicodeCompiler
