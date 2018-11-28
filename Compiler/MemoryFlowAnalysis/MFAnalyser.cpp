//
// Created by Theo Weidmann on 02.06.18.
//

#include "MFAnalyser.hpp"
#include "MFFunctionAnalyser.hpp"
#include "Package/Package.hpp"
#include "Types/Class.hpp"
#include "Types/Enum.hpp"
#include "Types/Protocol.hpp"
#include "Types/ValueType.hpp"

namespace EmojicodeCompiler {

void MFAnalyser::analyse() {
    for (auto &klass : package_->classes()) {
        analyseTypeDefinition(klass.get());
    }
    for (auto &vt : package_->valueTypes()) {
        analyseTypeDefinition(vt.get());
    }
    for (auto &function : package_->functions()) {
        analyseFunction(function.get());
    }
}

void MFAnalyser::analyseTypeDefinition(TypeDefinition *typeDef) {
    typeDef->eachFunction([this](Function *function) {
        analyseFunction(function);
    });
}

void MFAnalyser::analyseFunction(Function *function) {
    MFFunctionAnalyser(function).analyse();
}

}  // namespace EmojicodeCompiler
