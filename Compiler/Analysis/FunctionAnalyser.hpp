//
//  FunctionAnalyser.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef FunctionAnalyser_hpp
#define FunctionAnalyser_hpp

#include "ExpressionAnalyser.hpp"

namespace EmojicodeCompiler {

class ASTBlock;

/// This class is responsible for managing the semantic analysis of a function.
class FunctionAnalyser : public ExpressionAnalyser {
public:
    FunctionAnalyser(Function *function, SemanticAnalyser *analyser);

    FunctionAnalyser(Function *function, std::unique_ptr<SemanticScoper> scoper, SemanticAnalyser *analyser);

    /// Analyses the function.
    /// Analyses all statements.
    /// Checks that the method returns a value appropriately.
    /// If the function is an initializer, check that it initializes the instance correctly.
    void analyse();

    const Function* function() const { return function_; }

    void configureClosure(Function *closure) const override;

    void setInUnsafeBlock(bool v) { inUnsafeBlock_ = v; }
    bool isInUnsafeBlock() const override { return inUnsafeBlock_; }

    void checkThisUse(const SourcePosition &p) const override;
    FunctionType functionType() const override;

    /// Throws an error if an uninitialized variable is found in this scope.
    /// The error message is created by concatenating errorMessageFront, the variable name, and errorMessageBack.
    void uninitializedVariablesCheck(const SourcePosition &p, const char *errorMessageFront,
                                     const char *errorMessageBack) const;

    ~FunctionAnalyser();

private:
    Function *function_;

    bool inUnsafeBlock_;

    void analyseReturn(ASTBlock *root);
    bool analyseInitializationRequirements();

    void analyseBabyBottle();
    void initOptionalInstanceVariables();
};

}  // namespace EmojicodeCompiler

#endif /* FunctionAnalyser_hpp */
