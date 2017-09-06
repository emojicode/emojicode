//
//  ValueType.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 12/06/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "ValueType.hpp"
#include "../Functions/Function.hpp"
#include "../Functions/Initializer.hpp"

namespace EmojicodeCompiler {

void ValueType::prepareForSemanticAnalysis() {
    TypeDefinition::prepareForSemanticAnalysis();
    if (primitive_ && !instanceVariables().empty()) {
        throw CompilerError(position(), "A value type marked with ⚪️ cannot have instance variables.");
    }
    TypeDefinition::finalizeProtocols(Type(this, false));
}

llvm::GlobalVariable* ValueType::valueTypeMetaFor(const std::vector<Type> &genericArguments) {
    auto it = valueTypeMetas_.find(genericArguments);
    if (it != valueTypeMetas_.end()) {
        return it->second;
    }

    //        for (auto variable : instanceScope().map()) {
    //            variable.second.type().objectVariableRecords(variable.second.id(), &boxObjectVariableInformation_.back());
    //        }
    // TODO: fix
    return nullptr;
}

void ValueType::addValueTypeMetaFor(const std::vector<Type> &genericArguments, llvm::GlobalVariable *valueTypeMeta) {
    valueTypeMetas_.emplace(genericArguments, valueTypeMeta);
//    auto id = package()->app()->boxObjectVariableInformation().size();
//    genericIds_.emplace(genericArguments, id);
//    package()->app()->boxObjectVariableInformation().emplace_back();
}

}  // namespace EmojicodeCompiler
