//
//  BoxingLayerBuilder.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 14/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "BoxingLayerBuilder.hpp"
#include "../AST/ASTExpr.hpp"
#include "../AST/ASTLiterals.hpp"
#include "../AST/ASTMethod.hpp"
#include "../AST/ASTStatements.hpp"
#include "../Functions/BoxingLayer.hpp"
#include <memory>

namespace EmojicodeCompiler {

void buildBoxingLayerAst(BoxingLayer *layer) {
    auto p = layer->position();
    auto args = ASTArguments(p);

    for (size_t i = 0; i < layer->destinationArgumentTypes().size(); i++) {
        args.addArguments(std::make_shared<ASTGetVariable>(layer->arguments[i].variableName, p));
    }

    std::shared_ptr<ASTExpr> expr;
    if (layer->owningType().type() == TypeType::Callable) {
        expr = std::make_shared<ASTCallableCall>(std::make_shared<ASTThis>(p), args, p);
    }
    else {
        expr = std::make_shared<ASTMethod>(layer->destinationFunction()->name(), std::make_shared<ASTThis>(p), args, p);
    }

    std::shared_ptr<ASTBlock> block = std::make_shared<ASTBlock>(p);
    if (layer->returnType.type() == TypeType::NoReturn) {
        block->appendNode(std::make_shared<ASTExprStatement>(expr, p));
    }
    else {
        block->appendNode(std::make_shared<ASTReturn>(expr, p));
    }
    layer->setAst(block);
}

}  // namespace EmojicodeCompiler
