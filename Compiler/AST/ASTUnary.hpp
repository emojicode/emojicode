//
//  ASTUnary.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTUnary_hpp
#define ASTUnary_hpp

#include <utility>
#include "ASTExpr.hpp"

namespace EmojicodeCompiler {

class ASTIsError final : public ASTUnary {
    using ASTUnary::ASTUnary;
public:
    Type analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) override;
};

class ASTUnwrap final : public ASTUnaryMFForwarding {
    using ASTUnaryMFForwarding::ASTUnaryMFForwarding;
public:
    Type analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;

private:
    bool error_ = false;

    Value* generateErrorUnwrap(FunctionCodeGenerator *fg) const;
};

}  // namespace EmojicodeCompiler

#endif /* ASTUnary_hpp */
