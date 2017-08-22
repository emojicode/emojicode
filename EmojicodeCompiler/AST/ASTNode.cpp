//
//  ASTNode.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTNode.hpp"
#include "../Analysis/SemanticAnalyser.hpp"
#include "../Scoping/SemanticScoper.hpp"

namespace EmojicodeCompiler {

void ASTVariable::copyVariableAstInfo(const ResolvedVariable &resVar, SemanticAnalyser *analyser) {
    inInstanceScope_ = resVar.inInstanceScope;
    varId_ = resVar.variable.id();
    if (inInstanceScope_) {
        analyser->pathAnalyser().recordIncident(PathAnalyserIncident::UsedSelf);
    }
}

}  // namespace EmojicodeCompiler
