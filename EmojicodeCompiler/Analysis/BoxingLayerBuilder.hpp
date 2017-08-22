//
//  BoxingLayerBuilder.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 14/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef BoxingLayerBuilder_hpp
#define BoxingLayerBuilder_hpp

#include "../AST/ASTExpr.hpp"
#include "../AST/ASTLiterals.hpp"
#include "../AST/ASTMethod.hpp"
#include "../AST/ASTStatements.hpp"
#include "../BoxingLayer.hpp"

namespace EmojicodeCompiler {

/// Creates and AST representing the actions of the boxing layer and assigns it to the layer.
/// @attention The layer must be semantically analysed and transformed!
void buildBoxingLayerAst(BoxingLayer *layer);

} // namespace EmojicodeCompiler

#endif /* BoxingLayerBuilder_hpp */
