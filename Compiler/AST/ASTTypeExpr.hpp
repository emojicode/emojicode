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

/// Subclasses of this class represent type expressions. After analysis expressionType() contains the type the
/// expression represents. This is not a TypeType::TypeAsValue.
///
/// When generating type expressions, code to retrieve a type from a type value is written as necessary.
class ASTTypeExpr : public ASTExpr {
public:
    ASTTypeExpr(TypeAvailability av, const SourcePosition &p) : ASTExpr(p), availability_(av) {}
    TypeAvailability availability() const { return availability_; }
protected:
    TypeAvailability availability_;
};

class ASTTypeFromExpr final : public ASTTypeExpr {
public:
    ASTTypeFromExpr(std::shared_ptr<ASTExpr> value, const SourcePosition &p)
            : ASTTypeExpr(TypeAvailability::DynamicAndAvailable, p), expr_(std::move(value)) {}
    Type analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    std::shared_ptr<ASTExpr> expr_;
};

class ASTStaticType : public ASTTypeExpr {
public:
    ASTStaticType(Type type, TypeAvailability av, const SourcePosition &p)
            : ASTTypeExpr(av, p), type_(std::move(type)) {}
    Type analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override;
protected:
    Type type_;
};

class ASTInferType final : public ASTStaticType {
public:
    explicit ASTInferType(const SourcePosition &p)
            : ASTStaticType(Type::noReturn(), TypeAvailability::StaticAndUnavailable, p) {}
    Type analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) override;
    void toCode(Prettyprinter &pretty) const override;
};

class ASTThisType final : public ASTTypeExpr {
public:
    explicit ASTThisType(const SourcePosition &p) : ASTTypeExpr(TypeAvailability::DynamicAndAvailable, p) {}
    Type analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(Prettyprinter &pretty) const override;
};

} // namespace EmojicodeCompiler

#endif /* ASTTypeExpr_hpp */
