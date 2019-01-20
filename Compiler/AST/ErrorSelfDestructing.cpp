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
}

void ErrorSelfDestructing::buildDestruct(FunctionCodeGenerator *fg) const {
    if (!release_.empty()) {
        for (auto &release : release_) {
            fg->releaseByReference(fg->instanceVariablePointer(release.first), release.second);
        }
        auto clInf = fg->buildGetClassInfoFromObject(fg->thisValue());
        auto classInfo = dynamic_cast<const ClassReification*>(fg->typeHelper().ownerReification())->classInfo;
        fg->createIf(fg->builder().CreateICmpEQ(clInf, classInfo), [&] {
            fg->builder().CreateCall(fg->generator()->declarator().releaseMemory(),
                                     fg->builder().CreateBitCast(fg->thisValue(),
                                                                 llvm::Type::getInt8PtrTy(fg->generator()->context())));
        });
    }
}

}  // namespace EmojicodeCompiler
