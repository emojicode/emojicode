//
// Created by Theo Weidmann on 07.02.18.
//

#ifndef EMOJICODE_PROTOCOLSTABLEGENERATOR_HPP
#define EMOJICODE_PROTOCOLSTABLEGENERATOR_HPP

#include <cstddef>
#include <vector>
#include <Types/Type.hpp>

namespace llvm {
class Constant;
class GlobalVariable;
class LLVMContext;
class Module;
}  // namespace llvm

namespace EmojicodeCompiler {

class TypeDefinition;
class Protocol;
class LLVMTypeHelper;

class ProtocolsTableGenerator {
public:
    ProtocolsTableGenerator(llvm::LLVMContext &context, llvm::Module &module, const LLVMTypeHelper &typeHelper)
            : context_(context), module_(module), typeHelper_(typeHelper) {}
    void createProtocolsTable(const Type &type);
    void declareImportedProtocolsTable(const Type &type);

private:
    llvm::LLVMContext &context_;
    llvm::Module &module_;
    const LLVMTypeHelper &typeHelper_;

    /// Creates a virtual table (dispatch table) for the given protocol.
    /// @param protocol The protocol for which the virtual table is created.
    /// @param type The type definition from which methods will be dispatched.
    llvm::GlobalVariable* createVirtualTable(const Type &type, const EmojicodeCompiler::Type &protocol);

    llvm::GlobalVariable *getConformanceVariable(const Type &type, const Type &protocol, llvm::Constant *conformance) const;
};

}  // namespace EmojicodeCompiler

#endif //EMOJICODE_PROTOCOLSTABLEGENERATOR_HPP
