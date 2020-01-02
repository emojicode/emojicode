//
// Created by Theo Weidmann on 07.02.18.
//

#ifndef EMOJICODE_PROTOCOLSTABLEGENERATOR_HPP
#define EMOJICODE_PROTOCOLSTABLEGENERATOR_HPP

#include <cstddef>
#include <map>
#include "Types/Type.hpp"

namespace llvm {
class Constant;
class GlobalVariable;
}  // namespace llvm

namespace EmojicodeCompiler {

struct ProtocolConformance;
class TypeDefinition;
class CodeGenerator;

/// This class is responsible for declaring and generating individual protocol dispatch tables and the protocol table,
/// which can then be stored into the box info or class info for value types and class types repectivley.
///
/// The protocol dispatch table map the protocol method VTI’s to the function the type which conforms to the protocol
/// defined. One protocol table is created per type, which contains pointers to all protocols dispatch tables and
/// allows dynamic casting to protocols.
class ProtocolsTableGenerator {
public:
    ProtocolsTableGenerator(CodeGenerator *generator) : generator_(generator) {}

    /// Generates the protocol dispatch tables and the protocol table for the provided type.
    /// @pre If the type is a value type, the box info must have been set.
    void generate(const Type &type);
    /// Declares the protocol dispatch tables.
    void declareImported(const Type &type);

    /// Creates the protocol table for the provided TypeDefinition.
    /// @pre generate() must have been previously called for @c typeDef.
    llvm::Constant* createProtocolTable(TypeDefinition *typeDef);

    llvm::GlobalVariable* multiprotocol(const Type &multiprotocol, const Type &conformer);

private:
    CodeGenerator *generator_;
    std::map<std::pair<Type, TypeDefinition*>, llvm::GlobalVariable*> multiprotocolTables_;

    /// Creates a dispatch table for the given protocol.
    /// @param protocol The protocol for which the dispatch table is created.
    /// @param type The type definition from which methods will be dispatched.
    /// @param boxInfo The pointer to the box info for @c type that is stored in the protocol dispatch table.
    llvm::GlobalVariable* createDispatchTable(const Type &type, const ProtocolConformance &conformance,
                                              llvm::Constant *boxInfo);

    llvm::GlobalVariable* getConformanceVariable(const Type &type, const Type &protocol,
                                                 llvm::Constant *conformance) const;
};

}  // namespace EmojicodeCompiler

#endif //EMOJICODE_PROTOCOLSTABLEGENERATOR_HPP
