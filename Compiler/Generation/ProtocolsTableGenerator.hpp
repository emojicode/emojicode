//
// Created by Theo Weidmann on 07.02.18.
//

#ifndef EMOJICODE_PROTOCOLSTABLEGENERATOR_HPP
#define EMOJICODE_PROTOCOLSTABLEGENERATOR_HPP

#include <cstddef>
#include <vector>

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
    ProtocolsTableGenerator(llvm::LLVMContext &context, llvm::Module &module) : context_(context), module_(module) {}
    void createProtocolsTable(TypeDefinition *typeDef);

private:
    llvm::LLVMContext &context_;
    llvm::Module &module_;

    /// Creates a virtual table (dispatch table) for the given protocol.
    /// @param protocol The protocol for which the virtual table is created.
    /// @param typeDef The type definition from which methods will be dispatched.
    llvm::GlobalVariable* createVirtualTable(TypeDefinition *typeDef, Protocol *protocol);
};

}  // namespace EmojicodeCompiler

#endif //EMOJICODE_PROTOCOLSTABLEGENERATOR_HPP
