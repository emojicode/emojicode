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
#include "ASTNode.hpp"

namespace EmojicodeCompiler {

/// Type Expressions appear when a $type-expression$ is expected.
///
/// Subclasses of this class represent type expressions. After analysis expressionType() contains the type the
/// expression represents. This is not a TypeType::TypeAsValue.
///
/// When generating type expressions, code to retrieve a type from a type value is written as necessary.
class ASTTypeExpr : public ASTExpr {
public:
    ASTTypeExpr(const SourcePosition &p) : ASTExpr(p) {}
};

class ASTTypeFromExpr : public ASTTypeExpr {
public:
    ASTTypeFromExpr(std::shared_ptr<ASTExpr> value, const SourcePosition &p)
            : ASTTypeExpr(p), expr_(std::move(value)) {}

    Type analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) final;
    Value *generate(FunctionCodeGenerator *fg) const final;
    void toCode(Prettyprinter &pretty) const override;
private:
    std::shared_ptr<ASTExpr> expr_;
};

class ASTStaticType : public ASTTypeExpr {
public:
    ASTStaticType(Type type, const SourcePosition &p)
            : ASTTypeExpr(p), type_(std::move(type)) {}

    Type analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value *generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override;
protected:
    Type type_;
};

class ASTInferType final : public ASTStaticType {
public:
    explicit ASTInferType(const SourcePosition &p) : ASTStaticType(Type::noReturn(), p) {}

    Type analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) override;
    void toCode(Prettyprinter &pretty) const override;
};

class ASTThisType final : public ASTTypeFromExpr {
public:
    explicit ASTThisType(const SourcePosition &p);
    void toCode(Prettyprinter &pretty) const override;
};

}  // namespace EmojicodeCompiler

#endif /* ASTTypeExpr_hpp */
