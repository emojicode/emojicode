//
//  ASTTypeExpr.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTTypeExpr_hpp
#define ASTTypeExpr_hpp

#include "ASTNode.hpp"
#include "ASTExpr.hpp"

namespace EmojicodeCompiler {

class ASTTypeExpr : public ASTExpr {
public:
    ASTTypeExpr(TypeAvailability av, const SourcePosition &p) : ASTExpr(p), availability_(av) {}
    TypeAvailability availability() const { return availability_; }
protected:
    TypeAvailability availability_;
};

class ASTTypeFromExpr final : public ASTTypeExpr {
public:
    ASTTypeFromExpr(const std::shared_ptr<ASTExpr> &value, const SourcePosition &p)
    : ASTTypeExpr(TypeAvailability::DynamicAndAvailable, p), expr_(value) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
private:
    std::shared_ptr<ASTExpr> expr_;
};

class ASTStaticType : public ASTTypeExpr {
public:
    ASTStaticType(const Type &type, TypeAvailability av, const SourcePosition &p)
    : ASTTypeExpr(av, p), type_(type) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
protected:
    Type type_;
};

class ASTInferType final : public ASTStaticType {
public:
    ASTInferType(const SourcePosition &p) : ASTStaticType(Type::nothingness(), TypeAvailability::StaticAndUnavailable, p) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
};

class ASTThisType final : public ASTTypeExpr {
public:
    ASTThisType(const SourcePosition &p) : ASTTypeExpr(TypeAvailability::DynamicAndAvailable, p) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
};

}

#endif /* ASTTypeExpr_hpp */
