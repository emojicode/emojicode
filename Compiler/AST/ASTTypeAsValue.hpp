//
//  ASTTypeAsValue.hpp
//  runtime
//
//  Created by Theo Weidmann on 07.03.19.
//

#ifndef ASTTypeAsValue_hpp
#define ASTTypeAsValue_hpp

#include "ASTExpr.hpp"
#include "Lex/Token.hpp"

namespace EmojicodeCompiler {

class ASTTypeAsValue final : public ASTExpr {
public:
    ASTTypeAsValue(std::unique_ptr<ASTType> type, TokenType tokenType, const SourcePosition &p);
    Type analyse(ExpressionAnalyser *analyser) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) override {}

    ~ASTTypeAsValue();

private:
    std::unique_ptr<ASTType> type_;
    TokenType tokenType_;
};

}  // namespace EmojicodeCompiler

#endif /* ASTTypeAsValue_hpp */
