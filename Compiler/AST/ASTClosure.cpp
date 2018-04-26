//
//  ASTClosure.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 16/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTClosure.hpp"
#include "Analysis/ThunkBuilder.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Types/TypeDefinition.hpp"
#include "Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

ASTClosure::ASTClosure(std::unique_ptr<Function> &&closure, const SourcePosition &p)
        : ASTExpr(p), closure_(std::move(closure)) {}

Type ASTClosure::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    closure_->setMutating(analyser->function()->mutating());
    closure_->setOwningType(analyser->function()->owningType());

    applyBoxingFromExpectation(analyser, expectation);

    auto closureAnaly = FunctionAnalyser(closure_.get(), std::make_unique<CapturingSemanticScoper>(analyser->scoper()),
                                         analyser->semanticAnalyser());
    closureAnaly.analyse();
    capture_.captures = dynamic_cast<CapturingSemanticScoper &>(closureAnaly.scoper()).captures();
    if (closureAnaly.pathAnalyser().hasPotentially(PathAnalyserIncident::UsedSelf)) {
        capture_.captureSelf = true;
    }
    return Type(closure_.get());
}

void ASTClosure::applyBoxingFromExpectation(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    if (expectation.type() != TypeType::Callable ||
            expectation.genericArguments().size() - 1 != closure_->parameters().size()) {
        return;
    }

    auto expReturn = expectation.genericArguments().front();
    if (closure_->returnType().compatibleTo(expReturn, analyser->typeContext()) &&
            closure_->returnType().storageType() != expReturn.storageType()) {
        switch (expReturn.storageType()) {
            case StorageType::SimpleOptional:
                assert(closure_->returnType().storageType() == StorageType::Simple);
                closure_->setReturnType(Type(MakeOptional, closure_->returnType()));
                break;
            case StorageType::SimpleError:
                assert(closure_->returnType().storageType() == StorageType::Simple);
                closure_->setReturnType(Type(MakeError, expReturn.errorEnum(), closure_->returnType()));
                break;
            case StorageType::Simple:
                // We cannot deal with this, can we?
                break;
            case StorageType::Box: {
                Type g = closure_->returnType();
                g.forceBox();
                closure_->setReturnType(g);
                break;
            }
        }
    }

    auto shadowParams = closure_->parameters();
    for (size_t i = 0; i < closure_->parameters().size(); i++) {
        auto param = closure_->parameters()[i];
        auto expParam = expectation.genericArguments()[i + 1];
        if (param.type.compatibleTo(expParam, analyser->typeContext()) &&
                param.type.storageType() != expParam.storageType()) {
            switch (expParam.storageType()) {
                case StorageType::SimpleOptional:
                    assert(param.type.storageType() == StorageType::Simple);
                    shadowParams[i].type = Type(MakeOptional, param.type);
                    break;
                case StorageType::SimpleError:
                    assert(param.type.storageType() == StorageType::Simple);
                    shadowParams[i].type = Type(MakeError, expParam.errorEnum(), param.type);
                    break;
                case StorageType::Simple:
                    // We cannot deal with this, can we?
                    break;
                case StorageType::Box:
                    shadowParams[i].type.forceBox();
                    break;
            }
        }
    }
    closure_->setParameters(shadowParams);
}

}  // namespace EmojicodeCompiler
