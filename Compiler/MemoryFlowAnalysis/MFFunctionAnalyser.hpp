//
//  MFFunctionAnalyser.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 01/06/2018.
//  Copyright © 2018 Theo Weidmann. All rights reserved.
//

#ifndef MFAnalyser_hpp
#define MFAnalyser_hpp

#include "MFType.hpp"
#include "Scoping/IDScoper.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace EmojicodeCompiler {

class ASTExpr;
class ASTArguments;
class Function;
class MFHeapAllocates;
struct SemanticScopeStats;

struct MFLocalVariable {
    bool isParam = false;
    size_t param;
    MFType type = MFType::Borrowing;
    std::vector<MFHeapAllocates*> inits;
};

/// This class is responsible for managing the semantic analysis of a function.
class MFFunctionAnalyser {
public:
    MFFunctionAnalyser(Function *function);

    void analyse();
    void take(std::shared_ptr<ASTExpr> *expr);
    void recordThis(MFType type);
    void recordVariableGet(size_t id, MFType type);
    void recordVariableSet(size_t id, ASTExpr *expr, bool init);
    void recordReturn(std::shared_ptr<ASTExpr> *expr);
    void analyseFunctionCall(ASTArguments *node, ASTExpr *callee, Function *function);
    void popScope(const SemanticScopeStats &stats);

    const Function* function() const { return function_; }

private:
    IDScoper<MFLocalVariable> scope_;
    Function *function_;
    bool thisEscapes_ = false;

    void update(MFLocalVariable &variable);
};

}  // namespace EmojicodeCompiler

#endif /* FunctionAnalyser_hpp */
