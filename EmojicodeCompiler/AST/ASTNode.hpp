//
//  ASTNode.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTNode_hpp
#define ASTNode_hpp

#include "../Types/Type.hpp"
#include "../Types/TypeAvailability.hpp"
#include "../Lex/SourcePosition.hpp"
#include "../Scoping/VariableID.hpp"
#include <memory>

namespace EmojicodeCompiler {

struct ResolvedVariable;
class SemanticAnalyser;

class ASTNode {
public:
    ASTNode(const SourcePosition &p) : sourcePosition_(p) {}

    /// The source position that caused this node to be created
    const SourcePosition& position() const { return sourcePosition_; }
private:
    SourcePosition sourcePosition_;
};

class ASTVariable {
protected:
    bool inInstanceScope() { return inInstanceScope_; }
    VariableID varId() { return varId_; }
    void copyVariableAstInfo(const ResolvedVariable &, SemanticAnalyser *analyser);
protected:
    bool inInstanceScope_ = false;
    VariableID varId_;
};

}  // namespace Emojicode

#endif /* ASTNode_hpp */
