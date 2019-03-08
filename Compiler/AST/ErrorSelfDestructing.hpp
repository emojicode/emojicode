//
//  ErrorSelfDestructing.hpp
//  runtime
//
//  Created by Theo Weidmann on 24.09.18.
//

#ifndef ErrorSelfDestructing_hpp
#define ErrorSelfDestructing_hpp

#include "Types/Type.hpp"
#include <vector>

namespace llvm {
class Value;
}  // namespace llvm

namespace EmojicodeCompiler {

class FunctionAnalyser;
class FunctionCodeGenerator;
class ASTExpr;
struct SourcePosition;

/// This class encapsulates the logic of deinitializing an object when initialization is aborted by raising an error.
///
/// An object whose initialization is aborted by an error cannot be deinitialized by the deinitializer as it might
/// not be fully initialiized. Instead all instance variables that were indeed initialized must be released.
/// This is what this class does.
class ErrorSelfDestructing {
protected:
    /// Must be called during semantic analysis to determine which variables were already initialized.
    void analyseInstanceVariables(FunctionAnalyser *analyser, const SourcePosition &p);
    /// Builds the IR to release all instance variables that were initialized when analyseInstanceVariables() was
    /// called.
    void buildDestruct(FunctionCodeGenerator *fg) const;

private:
    std::vector<std::pair<size_t, Type>> release_;
    Class *class_ = nullptr;
};

class ErrorHandling {
protected:
    /// @pre This function must be called before generating `expr`.
    llvm::Value* prepareErrorDestination(FunctionCodeGenerator *fg, ASTExpr *expr) const;

    llvm::Value* isError(FunctionCodeGenerator *fg, llvm::Value *errorDestination) const;
};

}  // namespace EmojicodeCompiler

#endif /* ErrorSelfDestructing_hpp */
