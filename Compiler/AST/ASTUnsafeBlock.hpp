//
// Created by Theo Weidmann on 21.03.18.
//

#ifndef EMOJICODE_ASTUNSAFEBLOCK_HPP
#define EMOJICODE_ASTUNSAFEBLOCK_HPP

#include "ASTStatements.hpp"

namespace EmojicodeCompiler {

class ASTUnsafeBlock : public ASTStatement {
public:
    ASTUnsafeBlock(ASTBlock block, SourcePosition p) : ASTStatement(std::move(p)), block_(std::move(block)) {}

    void analyse(FunctionAnalyser *analyser) override;
    void generate(FunctionCodeGenerator *fg) const override { block_.generate(fg); }

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser) override {
        block_.analyseMemoryFlow(analyser);
    }
    
private:
    ASTBlock block_;
};

}  // namespace EmojicodeCompiler

#endif //EMOJICODE_ASTUNSAFEBLOCK_HPP
