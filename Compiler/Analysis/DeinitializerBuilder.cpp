//
//  DeinitializerBuilder.cpp
//  runtime
//
//  Created by Theo Weidmann on 30.08.18.
//

#include "DeinitializerBuilder.hpp"
#include "AST/ASTMemory.hpp"
#include "Functions/Function.hpp"
#include "Types/Class.hpp"
#include "Types/ValueType.hpp"
#include "Scoping/Scope.hpp"

namespace EmojicodeCompiler {

void buildDeinitializer(TypeDefinition *typeDef) {
    auto deinit = typeDef->deinitializer();

    if (deinit->ast() == nullptr) {
        deinit->setAst(std::make_unique<ASTBlock>(deinit->position()));
    }

    for (auto &decl : typeDef->instanceVariables()) {
        auto &var = typeDef->instanceScope().getLocalVariable(decl.name);
        if (!var.inherited() && var.type().isManaged()) {
            deinit->ast()->appendNode(std::make_unique<ASTRelease>(true, var.id(), var.type(), deinit->position()));
        }
    }

    if (auto klass = dynamic_cast<Class *>(typeDef)) {
        if (klass->superclass() != nullptr) {
            deinit->ast()->appendNode(std::make_unique<ASTSuperDeinitializer>(klass->superclass()->deinitializer(),
                                                                              deinit->position()));
        }
    }

    deinit->setMemoryFlowTypeForThis(MFFlowCategory::Borrowing);
}

void buildCopyRetain(ValueType *typeDef) {
    auto copyRetain = typeDef->copyRetain();

    copyRetain->setAst(std::make_unique<ASTBlock>(copyRetain->position()));

    for (auto &decl : typeDef->instanceVariables()) {
        auto &var = typeDef->instanceScope().getLocalVariable(decl.name);
        if (var.type().isManaged()) {
            copyRetain->ast()->appendNode(std::make_unique<ASTRetain>(true, var.id(), var.type(),
                                                                      copyRetain->position()));
        }
    }

    copyRetain->setMemoryFlowTypeForThis(MFFlowCategory::Borrowing);
}

} // namespace EmojicodeCompiler
