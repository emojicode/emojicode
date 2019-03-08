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
#include "AST/ASTExpr.hpp"

namespace EmojicodeCompiler {

void ErrorSelfDestructing::analyseInstanceVariables(FunctionAnalyser *analyser, const SourcePosition &p) {
    for (auto &var : analyser->scoper().instanceScope()->map()) {
        Variable &cv = var.second;
        auto incident = PathAnalyserIncident(true, cv.id());
        if (cv.type().isManaged() && analyser->pathAnalyser().hasPotentially(incident)) {
            release_.emplace_back(cv.id(), cv.type());
            if (!analyser->pathAnalyser().hasCertainly(incident)) {
                throw CompilerError(p, "Initialization cannot be aborted: Variable ", utf8(cv.name()),
                                    " must either be initialized on all paths or not at all.");
            }
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

llvm::Value* ErrorHandling::prepareErrorDestination(FunctionCodeGenerator *fg, ASTExpr *expr) const {
    auto call = dynamic_cast<ASTCall *>(expr);
    auto type = fg->typeHelper().llvmTypeFor(call->errorType());
    auto alloca = fg->createEntryAlloca(type, "error");
    fg->builder().CreateStore(llvm::Constant::getNullValue(type), alloca);
    call->setErrorPointer(alloca);
    return alloca;
}

llvm::Value* ErrorHandling::isError(FunctionCodeGenerator *fg, llvm::Value *errorDestination) const {
    auto type = llvm::dyn_cast<llvm::PointerType>(errorDestination->getType()->getPointerElementType());
    auto null = llvm::ConstantPointerNull::get(type);
    return fg->builder().CreateICmpNE(null, fg->builder().CreateLoad(errorDestination));
}

}  // namespace EmojicodeCompiler
