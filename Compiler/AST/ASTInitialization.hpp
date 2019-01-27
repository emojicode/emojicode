//
//  ASTInitialization.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTInitialization_hpp
#define ASTInitialization_hpp

#include "ASTExpr.hpp"
#include "MemoryFlowAnalysis/MFHeapAllocates.hpp"
#include <utility>

namespace EmojicodeCompiler {

enum class CallType;

class ASTInitialization final : public ASTExpr, public MFHeapAllocates {
public:
    enum class InitType {
        Enum, ValueType, Class, ClassStack, MemoryAllocation
    };

    ASTInitialization(std::u32string name, std::unique_ptr<ASTTypeExpr> type,
                      ASTArguments args, const SourcePosition &p);
    Type analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;
    /// @pre setDestination() must have been used to set a destination if initType() == InitType::ValueType
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) override;

    /// Sets the destination for a value type inititalization.
    ///
    /// A destination must be set via this setter before generating code for a value type initialization. The value
    /// must evaluate to a pointer of the type, which is to be initialized.
    /// @see ASTBoxing, SemanticAnalyser::comply(),
    void setDestination(llvm::Value *dest) { vtDestination_ = dest; }
    /// Returns the type of type which is initialized.
    InitType initType() { return initType_; }

    void allocateOnStack() override;

    static Value *initObject(FunctionCodeGenerator *fg, const ASTArguments &args, Function *function,
                             const Type &type, bool stackInit = false);

    ~ASTInitialization();

private:
    InitType initType_ = InitType::Class;
    std::u32string name_;
    std::unique_ptr<ASTTypeExpr> typeExpr_;
    llvm::Value *vtDestination_ = nullptr;
    Function *initializer_ = nullptr;
    ASTArguments args_;

    Value* generateClassInit(FunctionCodeGenerator *fg) const;
    Value* generateMemoryAllocation(FunctionCodeGenerator *fg) const;
    Value* generateInitValueType(FunctionCodeGenerator *fg) const;

    Type analyseEnumInit(ExpressionAnalyser *analyser, Type &type);
};

}  // namespace EmojicodeCompiler

#endif /* ASTInitialization_hpp */
