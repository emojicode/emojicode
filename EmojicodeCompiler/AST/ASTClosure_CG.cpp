//
//  ASTClosure_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTClosure.hpp"
#include "../Generation/ClosureCodeGenerator.hpp"
#include "../Generation/FnCodeGenerator.hpp"
#include "../Generation/STIProvider.hpp"

namespace EmojicodeCompiler {

void ASTClosure::generateExpr(FnCodeGenerator *fncg) const {
    auto closureGenerator = ClosureCodeGenerator(captures_, function_);
    closureGenerator.generate();

    fncg->wr().writeInstruction(INS_CLOSURE);
    function_->setVtiProvider(&STIProvider::globalStiProvider);
    function_->setUsed(false);
    function_->package()->registerFunction(function_);
    fncg->wr().writeInstruction(function_->vtiForUse());
    fncg->wr().writeInstruction(static_cast<EmojicodeInstruction>(captures_.size()));
    fncg->wr().writeInstruction(static_cast<EmojicodeInstruction>(closureGenerator.captureSize()));
    fncg->wr().writeInstruction(static_cast<EmojicodeInstruction>(closureGenerator.captureDestIndex()));

    auto objectVariableInformation = std::vector<ObjectVariableInformation>();
    objectVariableInformation.reserve(closureGenerator.captureSize());
    size_t index = 0;
    for (auto capture : captures_) {
        fncg->wr().writeInstruction(fncg->scoper().getVariable(capture.sourceId).stackIndex.value());
        fncg->wr().writeInstruction(capture.type.size());
        capture.type.objectVariableRecords(index, &objectVariableInformation);
        index += capture.type.size();
    }
    fncg->wr().writeInstruction(static_cast<EmojicodeInstruction>(objectVariableInformation.size()));
    for (auto &record : objectVariableInformation) {
        fncg->wr().writeInstruction((record.conditionIndex << 16) | static_cast<uint16_t>(record.index));
        fncg->wr().writeInstruction(static_cast<uint16_t>(record.type));
    }

    fncg->wr().writeInstruction(captureSelf_ ? 1 : 0);
}

}  // namespace EmojicodeCompiler
