//
//  ASTLiterals.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTLiterals_hpp
#define ASTLiterals_hpp

#include "ASTExpr.hpp"
#include <utility>

namespace EmojicodeCompiler {

class SemanticAnalyser;

class ASTStringLiteral final : public ASTExpr {
public:
    ASTStringLiteral(std::u32string value, const SourcePosition &p) : ASTExpr(p), value_(std::move(value)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    std::u32string value_;
    unsigned int varId_;
};

class ASTBooleanFalse final : public ASTExpr {
public:
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    explicit ASTBooleanFalse(const SourcePosition &p) : ASTExpr(p) {}
    Value* generate(FunctionCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override;
};

class ASTBooleanTrue final : public ASTExpr {
public:
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    explicit ASTBooleanTrue(const SourcePosition &p) : ASTExpr(p) {}
    Value* generate(FunctionCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override;
};

class ASTNumberLiteral final : public ASTExpr {
public:
    ASTNumberLiteral(double value, std::u32string string, const SourcePosition &p)
    : ASTExpr(p), string_(std::move(string)), doubleValue_(value), type_(NumberType::Double) {}
    ASTNumberLiteral(int64_t value, std::u32string string, const SourcePosition &p)
    : ASTExpr(p), string_(std::move(string)), integerValue_(value), type_(NumberType::Integer) {}

    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    enum class NumberType {
        Double, Integer
    };

    std::u32string string_;
    double doubleValue_ = 0;
    int64_t integerValue_ = 0;
    NumberType type_;
};

class ASTSymbolLiteral final : public ASTExpr {
public:
    ASTSymbolLiteral(char32_t value, const SourcePosition &p) : ASTExpr(p), value_(value) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    char32_t value_;
};

class ASTConcatenateLiteral final : public ASTExpr {
public:
    explicit ASTConcatenateLiteral(const SourcePosition &p) : ASTExpr(p) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void addValue(const std::shared_ptr<ASTExpr> &value) { values_.emplace_back(value); }
    Value* generate(FunctionCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    std::vector<std::shared_ptr<ASTExpr>> values_;
    VariableID varId_;
    Type type_ = Type::noReturn();
};

class ASTListLiteral final : public ASTExpr {
public:
    explicit ASTListLiteral(const SourcePosition &p) : ASTExpr(p) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void addValue(const std::shared_ptr<ASTExpr> &value) { values_.emplace_back(value); }
    Value* generate(FunctionCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    std::vector<std::shared_ptr<ASTExpr>> values_;
    VariableID varId_;
    Type type_ = Type::noReturn();
};

class ASTDictionaryLiteral final : public ASTExpr {
public:
    explicit ASTDictionaryLiteral(const SourcePosition &p) : ASTExpr(p) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void addValue(const std::shared_ptr<ASTExpr> &value) { values_.emplace_back(value); }
    Value* generate(FunctionCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    std::vector<std::shared_ptr<ASTExpr>> values_;
    VariableID varId_;
    Type type_ = Type::noReturn();
};

class ASTThis : public ASTExpr {
    using ASTExpr::ASTExpr;
public:
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override;
};

class ASTNothingness : public ASTExpr {
public:
    explicit ASTNothingness(const SourcePosition &p) : ASTExpr(p) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fncg) const override;
    void toCode(Prettyprinter &pretty) const override;
private:
    Type type_ = Type::noReturn();
};

}  // namespace EmojicodeCompiler

#endif /* ASTLiterals_hpp */
