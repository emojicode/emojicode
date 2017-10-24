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
#include "Types/Type.hpp"
#include "Types/TypeAvailability.hpp"
#include "Scoping/Variable.hpp"
#include <memory>
#include <utility>

namespace EmojicodeCompiler {

struct ResolvedVariable;
class SemanticAnalyser;

class ASTNode {
public:
    explicit ASTNode(SourcePosition p) : sourcePosition_(std::move(p)) {}

    /// The source position that caused this node to be created
    const SourcePosition& position() const { return sourcePosition_; }
private:
    SourcePosition sourcePosition_;
};

class ASTVariable {
protected:
    bool inInstanceScope() const { return inInstanceScope_; }
    VariableID varId() { return varId_; }
    void copyVariableAstInfo(const ResolvedVariable &, SemanticAnalyser *analyser);
protected:
    bool inInstanceScope_ = false;
    VariableID varId_;
};

}  // namespace EmojicodeCompiler

#endif /* ASTNode_hpp */
