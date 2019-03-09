//
//  Scope.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Scope.hpp"
#include "Compiler.hpp"
#include "CompilerError.hpp"
#include "Types/TypeDefinition.hpp"
#include "Analysis/PathAnalyser.hpp"

namespace EmojicodeCompiler {

Variable& Scope::declareVariable(const std::u32string &variable, const Type &type, bool constant,
                                 const SourcePosition &p) {
    return declareVariableWithId(variable, type, constant, VariableID(maxVariableId_++), p);
}

Variable& Scope::declareVariableWithId(const std::u32string &variable, Type type, bool constant, VariableID id,
                                       const SourcePosition &p) {
    if (hasLocalVariable(variable)) {
        throw CompilerError(p, "Cannot redeclare variable.");
    }
    type.setMutable(!constant);
    Variable &v = map_.emplace(variable, Variable(std::move(type), id, constant, variable, p)).first->second;
    return v;
}

Variable& Scope::getLocalVariable(const std::u32string &variable) {
    return map_.find(variable)->second;
}

bool Scope::hasLocalVariable(const std::u32string &variable) const {
    return map_.count(variable) > 0;
}

void Scope::checkScope(PathAnalyser *analyser, Compiler *compiler) const {
    for (auto &it : map_) {
        const Variable &cv = it.second;
        if (!cv.constant() && !cv.mutated() && !cv.inherited()) {
            compiler->warn(cv.position(), "Variable \"", utf8(cv.name()),
                           "\" was never mutated; consider making it a constant variable.");
        }
        auto incident = PathAnalyserIncident(false, cv.id());
        if (cv.type().type() != TypeType::Optional && analyser->hasPotentially(incident) &&
            !analyser->hasCertainly(incident)) {
            compiler->error(CompilerError(cv.position(), "Non-optional variable must be initialized on all paths. Move"
                                          " variable into a subscope where this applies or make it optional."));
        }
    }
}

}  // namespace EmojicodeCompiler
