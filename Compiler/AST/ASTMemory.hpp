//
//  ASTMemory.hpp
//  runtime
//
//  Created by Theo Weidmann on 17.07.18.
//

#ifndef ASTMemory_hpp
#define ASTMemory_hpp

#include "ASTExpr.hpp"
#include "ASTVariables.hpp"
#include <vector>
#include <memory>

namespace EmojicodeCompiler {

/// An ASTRelease node releases the content of the specified variable.
/// The variable must be of a type for which Type::isManaged() returns true.
class ASTRelease : public ASTStatement, AccessesAnyVariable {
public:
    ASTRelease(bool inInstanceScope, VariableID id, Type variableType, const SourcePosition &p)
        : ASTStatement(p), AccessesAnyVariable(inInstanceScope, id, std::move(variableType)) {
            assert(this->variableType().isManaged());
        }

    void analyseMemoryFlow(MFFunctionAnalyser *analyser) override {}
    void analyse(FunctionAnalyser *) final {}
    void toCode(PrettyStream &pretty) const override {}
    void generate(FunctionCodeGenerator *fg) const override;
};

/// An ASTRetain node retains the content of the specified variable.
/// The variable must be of a type for which Type::isManaged() returns true.
class ASTRetain : public ASTStatement, AccessesAnyVariable {
public:
    ASTRetain(bool inInstanceScope, VariableID id, Type variableType, const SourcePosition &p)
        : ASTStatement(p), AccessesAnyVariable(inInstanceScope, id, std::move(variableType)) {
            assert(this->variableType().isManaged());
        }

    void analyseMemoryFlow(MFFunctionAnalyser *analyser) override {}
    void analyse(FunctionAnalyser *) final {}
    void toCode(PrettyStream &pretty) const override {}
    void generate(FunctionCodeGenerator *fg) const override;
};

}

#endif /* ASTMemory_hpp */
