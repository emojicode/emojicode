//
//  ASTCast.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 25.09.18.
//

#ifndef ASTCast_hpp
#define ASTCast_hpp

#include "ASTExpr.hpp"

namespace EmojicodeCompiler {

class ASTCast final : public ASTUnaryMFForwarding {
public:
    ASTCast(std::shared_ptr<ASTExpr> value, std::unique_ptr<ASTTypeExpr> type,
            const SourcePosition &p);
    Type analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;

    ~ASTCast();

private:
    enum class CastType {
        ClassDowncast, ToClass, ToProtocol, ToValueType,
    };
    CastType castType_;
    std::unique_ptr<ASTTypeExpr> typeExpr_;
    Value* downcast(FunctionCodeGenerator *fg) const;
    Value* castToClass(FunctionCodeGenerator *fg, Value *box) const;
    Value* castToValueType(FunctionCodeGenerator *fg, Value *box) const;
    Value* castToProtocol(FunctionCodeGenerator *fg, Value *box) const;
    /// Returns the box info representing the type of information in the box. This includes fetching the box info
    /// from the protocol conformance if the box is a protocol box.
    Value* boxInfo(FunctionCodeGenerator *fg, Value *box) const;
};

}  // namespace EmojicodeCompiler

#endif /* ASTCast_hpp */
