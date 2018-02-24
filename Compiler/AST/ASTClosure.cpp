//
//  ASTClosure.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 16/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTClosure.hpp"
#include "Analysis/BoxingLayerBuilder.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Functions/BoxingLayer.hpp"
#include "Types/TypeDefinition.hpp"

namespace EmojicodeCompiler {

ASTClosure::ASTClosure(std::unique_ptr<Function> &&closure, const SourcePosition &p) :
    ASTExpr(p), closure_(std::move(closure)) {}

Type ASTClosure::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    closure_->setOwningType(analyser->function()->owningType());

    auto closureAnaly = FunctionAnalyser(closure_.get(), std::make_unique<CapturingSemanticScoper>(analyser->scoper()));
    closureAnaly.analyse();
    capture_.captures = dynamic_cast<CapturingSemanticScoper&>(closureAnaly.scoper()).captures();
    if (closureAnaly.pathAnalyser().hasPotentially(PathAnalyserIncident::UsedSelf)) {
        capture_.captureSelf = true;
    }
    return closure_->type();
}

}  // namespace EmojicodeCompiler
