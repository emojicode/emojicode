//
//  ASTClosure.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 16/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTClosure.hpp"
#include "Analysis/BoxingLayerBuilder.hpp"
#include "Analysis/SemanticAnalyser.hpp"
#include "Functions/BoxingLayer.hpp"
#include "Types/TypeDefinition.hpp"

namespace EmojicodeCompiler {

ASTClosure::ASTClosure(std::unique_ptr<Function> &&closure, const SourcePosition &p) :
    ASTExpr(p), closure_(std::move(closure)) {}

Type ASTClosure::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    closure_->setOwningType(analyser->function()->owningType());

    auto closureAnaly = SemanticAnalyser(closure_.get(), std::make_unique<CapturingSemanticScoper>(analyser->scoper()));
    closureAnaly.analyse();
    captures_ = dynamic_cast<CapturingSemanticScoper&>(closureAnaly.scoper()).captures();
    if (closureAnaly.pathAnalyser().hasPotentially(PathAnalyserIncident::UsedSelf)) {
        captureSelf_ = true;
    }
    return closure_->type();
}

}  // namespace EmojicodeCompiler
