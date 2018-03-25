//
// Created by Theo Weidmann on 26.02.18.
//

#ifndef EMOJICODE_SEMANTICANALYSER_HPP
#define EMOJICODE_SEMANTICANALYSER_HPP

#include <queue>

namespace EmojicodeCompiler {

class Function;
class Package;
class TypeDefinition;
class Type;
class TypeContext;
class Compiler;

/// Manages the semantic analysis of a package.
class SemanticAnalyser {
public:
    explicit SemanticAnalyser(Package *package, bool imported) : package_(package), imported_(imported) {}

    /// Analyses the package.
    /// @throws CompilerError if an unrecoverable error occurs, e.g. if the start flag function is not present but
    /// must be.
    /// @param executable True if this package will be linked to an executable. Requires, for instance, that a start
    /// flag function be present.
    void analyse(bool executable);

    void enqueueFunction(Function *);

    Compiler* compiler() const;

    /// Checks that no promises were broken and applies boxing if necessary.
    /// @returns false iff a value for protocol was given and the arguments or the return type are storage incompatible.
    /// This indicates that a BoxingLayer must be created.
    bool enforcePromises(const Function *sub, const Function *super, const Type &superSource,
                         const TypeContext &subContext, const TypeContext &superContext);

    void declareInstanceVariables(TypeDefinition *typeDef);
private:
    void analyseQueue();
    void enqueueFunctionsOfTypeDefinition(TypeDefinition *typeDef);
    void finalizeProtocols(const Type &type);
    void finalizeProtocol(const Type &type, const Type &protocol);
    void buildBoxingLayer(const Type &type, const Type &protocol, Function *method,
                          Function *methodImplementation);

    Package *package_;
    std::queue<Function *> queue_;
    bool imported_;

    bool checkArgumentPromise(const Function *sub, const Function *super, const TypeContext &subContext,
                                  const TypeContext &superContext) const;
};

}  // namespace EmojicodeCompiler

#endif //EMOJICODE_SEMANTICANALYSER_HPP
