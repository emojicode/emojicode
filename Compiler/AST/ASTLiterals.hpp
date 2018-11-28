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

class FunctionAnalyser;

class ASTStringLiteral final : public ASTExpr {
public:
    ASTStringLiteral(std::u32string value, const SourcePosition &p) : ASTExpr(p), value_(std::move(value)) {}
    Type analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) override {}

private:
    std::u32string value_;
    unsigned int varId_;
};

class ASTBooleanFalse final : public ASTExpr {
public:
    Type analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;
    explicit ASTBooleanFalse(const SourcePosition &p) : ASTExpr(p) {}
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override {}
};

class ASTBooleanTrue final : public ASTExpr {
public:
    Type analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;
    explicit ASTBooleanTrue(const SourcePosition &p) : ASTExpr(p) {}
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override {}
};

class ASTNumberLiteral final : public ASTExpr {
public:
    ASTNumberLiteral(double value, std::u32string string, const SourcePosition &p)
    : ASTExpr(p), string_(std::move(string)), doubleValue_(value), type_(NumberType::Double) {}
    ASTNumberLiteral(int64_t value, std::u32string string, const SourcePosition &p)
    : ASTExpr(p), string_(std::move(string)), integerValue_(value), type_(NumberType::Integer) {}

    Type analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override {}

private:
    enum class NumberType {
        Double, Integer, Byte
    };

    std::u32string string_;
    double doubleValue_ = 0;
    int64_t integerValue_ = 0;
    NumberType type_;
};

class ASTConcatenateLiteral final : public ASTExpr {
public:
    explicit ASTConcatenateLiteral(const SourcePosition &p) : ASTExpr(p) {}
    Type analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;
    void addValue(const std::shared_ptr<ASTExpr> &value) { values_.emplace_back(value); }
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override;

private:
    std::vector<std::shared_ptr<ASTExpr>> values_;
    Type type_ = Type::noReturn();
};

class ASTListLiteral final : public ASTExpr {
public:
    explicit ASTListLiteral(const SourcePosition &p) : ASTExpr(p) {}
    Type analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;
    void addValue(const std::shared_ptr<ASTExpr> &value) { values_.emplace_back(value); }
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override;

private:
    std::vector<std::shared_ptr<ASTExpr>> values_;
    Type type_ = Type::noReturn();
};

class ASTDictionaryLiteral final : public ASTExpr {
public:
    explicit ASTDictionaryLiteral(const SourcePosition &p) : ASTExpr(p) {}
    Type analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;
    void addValue(const std::shared_ptr<ASTExpr> &value) { values_.emplace_back(value); }
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override;

private:
    std::vector<std::shared_ptr<ASTExpr>> values_;
    Type type_ = Type::noReturn();
};

class ASTThis : public ASTExpr {
    using ASTExpr::ASTExpr;
public:
    Type analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override;
};

class ASTNoValue : public ASTExpr {
public:
    explicit ASTNoValue(const SourcePosition &p) : ASTExpr(p) {}
    Type analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override {}
    
private:
    Type type_ = Type::noReturn();
};

}  // namespace EmojicodeCompiler

#endif /* ASTLiterals_hpp */
