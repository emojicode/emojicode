//
//  ASTCast.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 25.09.18.
//

#ifndef ASTCast_hpp
#define ASTCast_hpp

#include "ASTUnary.hpp"

namespace llvm {
class Function;
}

namespace EmojicodeCompiler {

class CodeGenerator;

class ASTCast final : public ASTUnaryMFForwarding {
public:
    ASTCast(std::shared_ptr<ASTExpr> value, std::shared_ptr<ASTTypeExpr> type,
            const SourcePosition &p) : ASTUnaryMFForwarding(std::move(value), p), typeExpr_(std::move(type)) {}
    Type analyse(ExpressionAnalyser *analyser) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;

    static llvm::Function* getCastFunction(CodeGenerator *cg);
private:
    /// If this is true, this is a class downcast. Otherwise a dynamic cast.
    bool isDowncast_ = false;
    std::shared_ptr<ASTTypeExpr> typeExpr_;
    Value* downcast(FunctionCodeGenerator *fg) const;

    static llvm::Function *kFunction;

    static Value* castToClass(FunctionCodeGenerator *fg, Value *box, Value *typeDescription, Value *boxInfo,
                              llvm::Value *rtti);
    static Value* castToValueType(FunctionCodeGenerator *fg, Value *box, Value *typeDescription, Value *flag,
                                  Value *boxInfo, llvm::Value *rtti);
    static Value* castToProtocol(FunctionCodeGenerator *fg, Value *box, Value *rtti, Value *boxInfo);
    /// Returns the box info representing the type of information in the box. This includes fetching the box info
    /// from the protocol conformance if the box is a protocol box.
    Value* boxInfo(FunctionCodeGenerator *fg, Value *box) const;
};

}  // namespace EmojicodeCompiler

#endif /* ASTCast_hpp */
