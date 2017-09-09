//
//  ASTClosure.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 16/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTClosure.hpp"
#include "../Analysis/SemanticAnalyser.hpp"
#include "../Functions/Function.hpp"

namespace EmojicodeCompiler {

Type ASTClosure::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    function_->setOwningType(analyser->function()->owningType());
    function_->setFunctionType(analyser->function()->functionType());

    auto closureAnalyser = SemanticAnalyser(function_, std::make_unique<CapturingSemanticScoper>(analyser->scoper()));
    closureAnalyser.analyse();
    captures_ = dynamic_cast<CapturingSemanticScoper&>(closureAnalyser.scoper()).captures();
    if (closureAnalyser.pathAnalyser().hasPotentially(PathAnalyserIncident::UsedSelf)) {
        captureSelf_ = true;
    }
    return function_->type();
}

}  // namespace EmojicodeCompiler
