//
//  ASTLiterals.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//🍺

#ifndef ASTLiterals_hpp
#define ASTLiterals_hpp

#include "ASTExpr.hpp"
#include <utility>
#include <llvm/IR/Type.h>

namespace EmojicodeCompiler {

class FunctionAnalyser;

class ASTStringLiteral final : public ASTExpr {
public:
    ASTStringLiteral(std::u32string value, const SourcePosition &p) : ASTExpr(p), value_(std::move(value)) {}
    Type analyse(ExpressionAnalyser *analyser) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) override {}

private:
    std::u32string value_;
};

class ASTCGUTF8Literal final : public ASTExpr {
public:
    ASTCGUTF8Literal(std::string value, const SourcePosition &p) : ASTExpr(p), value_(std::move(value)) {}
    Type analyse(ExpressionAnalyser *analyser) override { return Type::noReturn(); }
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override {}
    void analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) override {}

private:
    std::string value_;
};

class ASTBooleanFalse final : public ASTExpr {
public:
    Type analyse(ExpressionAnalyser *analyser) override;
    explicit ASTBooleanFalse(const SourcePosition &p) : ASTExpr(p) {}
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override {}
};

class ASTBooleanTrue final : public ASTExpr {
public:
    Type analyse(ExpressionAnalyser *analyser) override;
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

    Type analyse(ExpressionAnalyser *analyser) override;
    Value* generate(FunctionCodeGenerator *fg) const override;
    Type comply(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;

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

class ASTCollectionLiteral : public ASTExpr {
public:
    explicit ASTCollectionLiteral(const SourcePosition &p) : ASTExpr(p) {}

    void addValue(const std::shared_ptr<ASTExpr> &value) { values_.emplace_back(value); }

    std::pair<Value*, Value*> prepareValueArray(FunctionCodeGenerator *fg, llvm::Type *type, size_t count, const char *name) const;
protected:
    std::vector<std::shared_ptr<ASTExpr>> values_;
    Type type_ = Type::noReturn();
    Initializer *initializer_ = nullptr;
    Value *init(FunctionCodeGenerator *fg, std::vector<llvm::Value *> args) const;
};

class ASTConcatenateLiteral final : public ASTExpr {
public:
    explicit ASTConcatenateLiteral(const SourcePosition &p) : ASTExpr(p) {}
    Type analyse(ExpressionAnalyser *analyser) override;
    void addValue(const std::shared_ptr<ASTExpr> &value) { values_.emplace_back(value); }
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override;

private:
    std::vector<std::shared_ptr<ASTExpr>> values_;
    Initializer *init_ = nullptr;
    Function *append_ = nullptr;
    Function *get_ = nullptr;
};

class ASTListLiteral final : public ASTCollectionLiteral {
public:
    explicit ASTListLiteral(const SourcePosition &p) : ASTCollectionLiteral(p) {}
    Type analyse(ExpressionAnalyser *analyser) override;
    Type comply(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;

    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override;
};

class ASTDictionaryLiteral final : public ASTCollectionLiteral {
public:
    explicit ASTDictionaryLiteral(const SourcePosition &p) : ASTCollectionLiteral(p) {}
    Type analyse(ExpressionAnalyser *analyser) override;
    Type comply(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;

    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override;
};

class ASTThis : public ASTExpr {
    using ASTExpr::ASTExpr;
public:
    Type analyse(ExpressionAnalyser *analyser) override;
    Value* generate(FunctionCodeGenerator *fg) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override;
};

class ASTNoValue : public ASTExpr {
public:
    explicit ASTNoValue(const SourcePosition &p) : ASTExpr(p) {}
    Type analyse(ExpressionAnalyser *analyser) override;
    Value* generate(FunctionCodeGenerator *fg) const override;
    Type comply(ExpressionAnalyser *analyser, const TypeExpectation &expectation) override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *, MFFlowCategory) override {}
    
private:
    Type type_ = Type::noReturn();
};

}  // namespace EmojicodeCompiler

#endif /* ASTLiterals_hpp */
