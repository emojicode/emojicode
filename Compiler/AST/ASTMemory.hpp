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

/// ASTSuperDeinitializer is used in deinitializers to call the superclass’ deinitializer. The superclass’
/// deinitializer is statically dispatched.
class ASTSuperDeinitializer : public ASTStatement {
public:
    /// @param deinit The superclass’ deinitializer.
    ASTSuperDeinitializer(Function *deinit, const SourcePosition &p) : ASTStatement(p), deinit_(deinit) {}

    void analyseMemoryFlow(MFFunctionAnalyser *analyser) override {}
    void analyse(FunctionAnalyser *) final {}
    void toCode(PrettyStream &pretty) const override {}
    void generate(FunctionCodeGenerator *fg) const override;

private:
    Function *deinit_;
};

}

#endif /* ASTMemory_hpp */
