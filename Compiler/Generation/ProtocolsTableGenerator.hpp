//
// Created by Theo Weidmann on 07.02.18.
//

#ifndef EMOJICODE_PROTOCOLSTABLEGENERATOR_HPP
#define EMOJICODE_PROTOCOLSTABLEGENERATOR_HPP

#include <cstddef>
#include <vector>
#include <map>
#include "Types/Type.hpp"

namespace llvm {
class Constant;
class GlobalVariable;
class LLVMContext;
class Module;
}  // namespace llvm

namespace EmojicodeCompiler {

class TypeDefinition;
class Protocol;
class CodeGenerator;

class BoxInfoGenerator {
public:
    BoxInfoGenerator(const Type &type, CodeGenerator *generator);

    void add(const Type &protocol, llvm::GlobalVariable *conformance);
    void finish();

private:
    CodeGenerator *const generator_;
    const Type &type_;
    std::vector<llvm::Constant *> boxInfos_;
};

class ProtocolsTableGenerator {
public:
    ProtocolsTableGenerator(CodeGenerator *generator) : generator_(generator) {}
    void createProtocolsTable(const Type &type);
    void declareImportedProtocolsTable(const Type &type);
    llvm::GlobalVariable* multiprotocol(const Type &multiprotocol, const Type &conformer);

private:
    CodeGenerator *generator_;
    std::map<std::pair<Type, TypeDefinition*>, llvm::GlobalVariable*> multiprotocolTables_;

    /// Creates a virtual table (dispatch table) for the given protocol.
    /// @param protocol The protocol for which the virtual table is created.
    /// @param type The type definition from which methods will be dispatched.
    llvm::GlobalVariable* createVirtualTable(const Type &type, const Type &protocol);

    llvm::GlobalVariable *getConformanceVariable(const Type &type, const Type &protocol, llvm::Constant *conformance) const;
};

}  // namespace EmojicodeCompiler

#endif //EMOJICODE_PROTOCOLSTABLEGENERATOR_HPP
