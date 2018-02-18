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
#include <utility>

namespace EmojicodeCompiler {

class ASTInitialization final : public ASTExpr {
public:
    enum class InitType {
        Enum, ValueType, Class, MemoryAllocation
    };

    ASTInitialization(std::u32string name, std::shared_ptr<ASTTypeExpr> type,
                      ASTArguments args, const SourcePosition &p)
    : ASTExpr(p), name_(std::move(name)), typeExpr_(std::move(type)), args_(std::move(args)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    /// @pre setDestination() must have been used to set a destination if initType() == InitType::ValueType
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override;

    /// Sets the destination for a value type inititalization.
    ///
    /// A destination must be set via this setter before generating code for a value type initialization. The value
    /// must evaluate to a pointer of the type, which is to be initialized.
    /// @see ASTBoxing, SemanticAnalyser::comply(),
    void setDestination(llvm::Value *dest) { vtDestination_ = dest; }
    /// Returns the type of type which is initialized.
    InitType initType() { return initType_; }

    static Value *initObject(FunctionCodeGenerator *fg, const ASTArguments &args, const std::u32string &name,
                                 const Type &type);
private:
    InitType initType_ = InitType::Class;
    std::u32string name_;
    std::shared_ptr<ASTTypeExpr> typeExpr_;
    llvm::Value *vtDestination_ = nullptr;
    ASTArguments args_;

    Value* generateClassInit(FunctionCodeGenerator *fg) const;
    Value* generateMemoryAllocation(FunctionCodeGenerator *fg) const;
};

}  // namespace EmojicodeCompiler

#endif /* ASTInitialization_hpp */
