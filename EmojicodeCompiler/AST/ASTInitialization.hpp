//
//  ASTInitialization.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTInitialization_hpp
#define ASTInitialization_hpp

#include <utility>
#include "ASTVariables.hpp"

namespace EmojicodeCompiler {

class ASTInitialization final : public ASTExpr {
public:
    enum class InitType {
        Enum, ValueType, Class
    };

    ASTInitialization(std::u32string name, std::shared_ptr<ASTTypeExpr> type,
                      ASTArguments args, const SourcePosition &p)
    : ASTExpr(p), name_(std::move(name)), typeExpr_(std::move(type)), args_(std::move(args)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override;

    void setDestination(const std::shared_ptr<ASTGetVariable> &dest) { vtDestination_ = dest; }
    InitType initType() { return initType_; }
private:
    InitType initType_ = InitType::Class;
    std::u32string name_;
    std::shared_ptr<ASTTypeExpr> typeExpr_;
    std::shared_ptr<ASTGetVariable> vtDestination_;
    ASTArguments args_;

    Value* generateClassInit(FunctionCodeGenerator *fg) const;
};

}  // namespace EmojicodeCompiler

#endif /* ASTInitialization_hpp */
