//
//  ASTClosure_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTClosure.hpp"
#include "../Generation/ClosureCodeGenerator.hpp"
#include "../Generation/FunctionCodeGenerator.hpp"

namespace EmojicodeCompiler {

Value* ASTClosure::generate(FunctionCodeGenerator *fg) const {
    // TODO: implement
//    auto closureGenerator = ClosureCodeGenerator(captures_, function_, fg->generator());
//    closureGenerator.generate();
//
//    fg->wr().writeInstruction(INS_CLOSURE);
//    function_->setVtiProvider(&STIProvider::globalStiProvider);
//    function_->setUsed(false);
//    function_->package()->registerFunction(function_);
//    fg->wr().writeInstruction(function_->vtiForUse());
//    fg->wr().writeInstruction(static_cast<EmojicodeInstruction>(captures_.size()));
//    fg->wr().writeInstruction(static_cast<EmojicodeInstruction>(closureGenerator.captureSize()));
//    fg->wr().writeInstruction(static_cast<EmojicodeInstruction>(closureGenerator.captureDestIndex()));
//
//    auto objectVariableInformation = std::vector<ObjectVariableInformation>();
//    objectVariableInformation.reserve(closureGenerator.captureSize());
//    size_t index = 0;
//    for (auto capture : captures_) {
//        fg->wr().writeInstruction(fg->scoper().getVariable(capture.sourceId).stackIndex.value());
//        fg->wr().writeInstruction(capture.type.size());
//        capture.type.objectVariableRecords(index, &objectVariableInformation);
//        index += capture.type.size();
//    }
//    fg->wr().writeInstruction(static_cast<EmojicodeInstruction>(objectVariableInformation.size()));
//    for (auto &record : objectVariableInformation) {
//        fg->wr().writeInstruction((record.conditionIndex << 16) | static_cast<uint16_t>(record.index));
//        fg->wr().writeInstruction(static_cast<uint16_t>(record.type));
//    }
//
//    fg->wr().writeInstruction(captureSelf_ ? 1 : 0);
    return nullptr;
}

}  // namespace EmojicodeCompiler
