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

namespace EmojicodeCompiler {

class SemanticAnalyser;

class ASTStringLiteral final : public ASTExpr {
public:
    ASTStringLiteral(const EmojicodeString &value, const SourcePosition &p) : ASTExpr(p), value_(value) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
private:
    EmojicodeString value_;
    unsigned int varId_;
};

class ASTBooleanFalse final : public ASTExpr {
public:
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    ASTBooleanFalse(const SourcePosition &p) : ASTExpr(p) {}
    void generateExpr(FnCodeGenerator *fncg) const override;
};

class ASTBooleanTrue final : public ASTExpr {
public:
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    ASTBooleanTrue(const SourcePosition &p) : ASTExpr(p) {}
    void generateExpr(FnCodeGenerator *fncg) const override;
};

class ASTNumberLiteral final : public ASTExpr {
public:
    ASTNumberLiteral(double value,
                     const SourcePosition &p) : ASTExpr(p), doubleValue_(value), type_(NumberType::Double) {}
    ASTNumberLiteral(int64_t value,
                     const SourcePosition &p) : ASTExpr(p), integerValue_(value), type_(NumberType::Integer) {}

    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
private:
    enum class NumberType {
        Double, Integer
    };

    double doubleValue_;
    int64_t integerValue_;
    NumberType type_;
};

class ASTSymbolLiteral final : public ASTExpr {
public:
    ASTSymbolLiteral(EmojicodeChar value, const SourcePosition &p) : ASTExpr(p), value_(value) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
private:
    EmojicodeChar value_;
};

class ASTConcatenateLiteral final : public ASTExpr {
public:
    ASTConcatenateLiteral(const SourcePosition &p) : ASTExpr(p) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void addValue(const std::shared_ptr<ASTExpr> &value) { values_.emplace_back(value); }
    void generateExpr(FnCodeGenerator *fncg) const override;
private:
    std::vector<std::shared_ptr<ASTExpr>> values_;
    VariableID varId_;
    Type type_ = Type::nothingness();
};

class ASTListLiteral final : public ASTExpr {
public:
    ASTListLiteral(const SourcePosition &p) : ASTExpr(p) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void addValue(const std::shared_ptr<ASTExpr> &value) { values_.emplace_back(value); }
    void generateExpr(FnCodeGenerator *fncg) const override;
private:
    std::vector<std::shared_ptr<ASTExpr>> values_;
    VariableID varId_;
    Type type_ = Type::nothingness();
};

class ASTDictionaryLiteral final : public ASTExpr {
    using ASTExpr::ASTExpr;
public:
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void addValue(const std::shared_ptr<ASTExpr> &value) { values_.emplace_back(value); }
    void generateExpr(FnCodeGenerator *fncg) const override;
private:
    std::vector<std::shared_ptr<ASTExpr>> values_;
};

class ASTThis : public ASTExpr {
    using ASTExpr::ASTExpr;
public:
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
};

class ASTNothingness : public ASTExpr {
    using ASTExpr::ASTExpr;
public:
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
};

}

#endif /* ASTLiterals_hpp */
