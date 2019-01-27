//
//  ASTTypeExpr.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTTypeExpr_hpp
#define ASTTypeExpr_hpp

#include <utility>
#include "ASTExpr.hpp"

namespace EmojicodeCompiler {

/// Type Expressions appear when a $type-expression$ is expected.
///
/// Subclasses of this class represent type expressions.
///
/// When generating type expressions, code to retrieve a type from a type value is written as necessary.
class ASTTypeExpr : public ASTNode {
public:
    ASTTypeExpr(const SourcePosition &p) : ASTNode(p) {}
    virtual Value* generate(FunctionCodeGenerator *fg) const = 0;

    const Type& analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation);
    /// Returns the type determined during analysis.
    /// @pre analyse() must have been called.
    const Type& type() const { return type_; }
private:
    virtual Type analyseImpl(ExpressionAnalyser *analyser, const TypeExpectation &expectation) = 0;
    Type type_ = Type::noReturn();
};

class ASTTypeFromExpr : public ASTTypeExpr {
public:
    ASTTypeFromExpr(std::shared_ptr<ASTExpr> value, const SourcePosition &p)
            : ASTTypeExpr(p), expr_(std::move(value)) {}

    Type analyseImpl(ExpressionAnalyser *analyser, const TypeExpectation &expectation) final;
    Value *generate(FunctionCodeGenerator *fg) const final;
    void toCode(PrettyStream &pretty) const override;
private:
    std::shared_ptr<ASTExpr> expr_;
};

class ASTStaticType : public ASTTypeExpr {
public:
    ASTStaticType(std::unique_ptr<ASTType> type, const SourcePosition &p)
            : ASTTypeExpr(p), type_(std::move(type)) {}

    Type analyseImpl(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value *generate(FunctionCodeGenerator *fg) const override;
    void toCode(PrettyStream &pretty) const override;
protected:
    std::unique_ptr<ASTType> type_;
};

class ASTInferType final : public ASTStaticType {
public:
    explicit ASTInferType(const SourcePosition &p) : ASTStaticType(nullptr, p) {}

    Type analyseImpl(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;
    void toCode(PrettyStream &pretty) const override;
};

class ASTThisType final : public ASTTypeFromExpr {
public:
    explicit ASTThisType(const SourcePosition &p);

    void toCode(PrettyStream &pretty) const override;
};

}  // namespace EmojicodeCompiler

#endif /* ASTTypeExpr_hpp */
