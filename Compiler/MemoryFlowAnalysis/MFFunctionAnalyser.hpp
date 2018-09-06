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
#include "Types/Type.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace EmojicodeCompiler {

class ASTExpr;
class ASTArguments;
class ASTBlock;
class Function;
class MFHeapAllocates;
struct SemanticScopeStats;

struct MFLocalVariable {
    bool isParam = false;
    bool isReturned = false;
    size_t param;
    MFType mfType = MFType::Borrowing;
    Type type = Type::noReturn();
    std::vector<MFHeapAllocates *> inits;
};

/// This class is responsible for managing the semantic analysis of a function.
class MFFunctionAnalyser {
public:
    MFFunctionAnalyser(Function *function);

    /// Analyses the function for which this MFFunctionAnalyser was created. This method must only be called once.
    void analyse();

    /// Retains the object to which the expression evaluates if it is an object.
    /// Analyses the expression as escaping.
    void retain(std::shared_ptr<ASTExpr> *expr);
    /// Records that the context value is used as value of the provided type.
    void recordThis(MFType type);
    void recordVariableGet(size_t id, MFType type);
    void recordVariableSet(size_t id, ASTExpr *expr, bool init, Type type);
    void recordReturn(std::shared_ptr<ASTExpr> *expr);
    void analyseFunctionCall(ASTArguments *node, ASTExpr *callee, Function *function);
    void popScope(ASTBlock *block);

    /// Returns the function that is being analysed.
    const Function* function() const { return function_; }

private:
    IDScoper<MFLocalVariable> scope_;
    Function *function_;
    bool thisEscapes_ = false;
};

}  // namespace EmojicodeCompiler

#endif /* FunctionAnalyser_hpp */
