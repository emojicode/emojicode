//
//  ASTNode.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTNode_hpp
#define ASTNode_hpp

#include "Lex/SourcePosition.hpp"

namespace EmojicodeCompiler {

class FunctionAnalyser;
class PrettyStream;

/// ASTNode is the parent class of all nodes in the abstract syntax tree.
class ASTNode {
public:
    explicit ASTNode(SourcePosition p) : sourcePosition_(p) {}

    /// The source position that caused this node to be created.
    const SourcePosition& position() const { return sourcePosition_; }
    /// Appends code that leads to the creation of a node like this to the provided Prettyprinter.
    virtual void toCode(PrettyStream &pretty) const = 0;

    virtual ~ASTNode() = default;
private:
    SourcePosition sourcePosition_;
};

}  // namespace EmojicodeCompiler

#endif /* ASTNode_hpp */
