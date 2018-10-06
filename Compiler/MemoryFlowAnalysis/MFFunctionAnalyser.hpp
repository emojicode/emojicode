//
//  MFFunctionAnalyser.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 01/06/2018.
//  Copyright © 2018 Theo Weidmann. All rights reserved.
//

#ifndef MFAnalyser_hpp
#define MFAnalyser_hpp

#include "MFFlowCategory.hpp"
#include "Scoping/IDScoper.hpp"
#include "Types/Type.hpp"
#include <utility>
#include <vector>

namespace EmojicodeCompiler {

class ASTExpr;
class ASTArguments;
class ASTBlock;
class Function;
class MFHeapAllocates;
struct SemanticScopeStats;

/// This class is responsible for analysing the memory flow of a function.
///
/// Memory flow analysis is not an optimization step but an integral part of Emojicode memory management.
///
/// When analysing memory flow, Emojicode divides every use of a value into two *flow categories*:
/// - Borrowing Use: The value is never used in a way that could let the value escape. "Escape"
///   means any action that makes the value available outside stack memory, e.g. an instance variable assignment or
///   otherwise storing the value into heap memory. Note that this does not indicate whether the object will ever be
///   retained or not.
/// - Escaping Use: The value could escape as described above. This category is also used if the compiler cannot say for
///   sure that the use of the value is Borrowing.
///
/// In order to classify all uses of a value as accurately as possible, MFFunctionAnalyser exists. One of its key jobs
/// is to determine the flow category of every parameter and to set the MFFlowCategory of every parameter.
///
/// The flow category can then be used to perform certain optimization like allocating values on the stack instead of
/// on the heap.
///
/// Furthemore, to manage allocation Emojicode distinguishes every value created into two *value categories*:
/// - Temporary Value: The value is only retained for the duration of the statement. It is to be released at the end
///   of the statement. For example, an object created and immediately passed as an argument will be released at the
///   end of the statement. A temporary value, however, is not necessarily created by initialization. Retrieving a
///   variable also creates a temporary value.
/// - Taken Value: A taken value does not need to be released at the end of the statement. This, for instance, is the
///   case if the value was assigned to a variable.
///
/// There is no direct connection between the *value category* and the *flow category* of a value. All combinations of
/// the categories are possible. For example, a temporary value could be passed to a Borrowing or an
/// Escaping parameter, while a taken value could be a value assigned to an instance variable (escaping) or to a
/// constant local variable (borrowing).
class MFFunctionAnalyser {
public:
    MFFunctionAnalyser(Function *function);

    /// Analyses the function for which this MFFunctionAnalyser was created. This method must only be called once.
    void analyse();

    /// Marks the value that is created by the expression as a Taken Value.
    /// @note This method does not affect the flow category of the value.
    void take(ASTExpr *expr);

    /// Records the flow category of a use of the context.
    void recordThis(MFFlowCategory category);
    /// Records the flow category of the use of a variable value.
    void recordVariableGet(size_t id, MFFlowCategory category);
    /// Records an expression whose resulting value was assigned to a variable.
    /// If the compiler can prove that the variable value is never used in an Escaping manner it will inform the
    /// expression that it can allocate on the heap if it inherits from MFHeapAllocates.
    /// Analyses expr as Escaping.
    /// @param expr The expression which is stored into the variable.
    ///             This value can be `nullptr` in special circumstances.
    /// @param type The type of the variable.
    void recordVariableSet(size_t id, ASTExpr *expr, Type type);

    /// Analyses a function call.
    /// Analyses the callee and arguments with the appropriate flow category. If the specified function was not
    /// memory flow analysed, analyses the function first.
    void analyseFunctionCall(ASTArguments *node, ASTExpr *callee, Function *function);

    /// This method must be called after having analysed an ASTBlock.
    /// It appropriately releases variable values, updates the MFFlowCategory of parameters and converts heap into
    /// stack allocations.
    void popScope(ASTBlock *block);

private:
    struct MFLocalVariable {
        bool isParam = false;
        bool isReturned = false;
        size_t param;
        MFFlowCategory flowCategory = MFFlowCategory::Borrowing;
        Type type = Type::noReturn();
        std::vector<MFHeapAllocates *> inits;
    };

    IDScoper<MFLocalVariable> scope_;
    Function *function_;
    bool thisEscapes_ = false;

    void releaseVariables(ASTBlock *block);
};

}  // namespace EmojicodeCompiler

#endif /* FunctionAnalyser_hpp */
