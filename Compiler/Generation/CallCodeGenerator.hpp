//
//  CallCodeGenerator.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef CallCodeGenerator_hpp
#define CallCodeGenerator_hpp

#include "Functions/CallType.h"
#include <string>
#include <vector>
#include <llvm/IR/Instructions.h>

namespace llvm {
class Value;
}  // namespace llvm

namespace EmojicodeCompiler {

class FunctionCodeGenerator;
class Type;
class Function;
class ASTArguments;

/// This class is responsible for generating IR to dispatch a provided method, initializer, type method or
/// protocol method.
///
/// It does all the heavy lifting concerning dispatch, dispatch tables etc.
///
/// It is also capable of generating code to find a protocol conformance if the callee of a dynamic protocol dispatch is
/// not boxed for the protocol in question.
class CallCodeGenerator {
public:
    CallCodeGenerator(FunctionCodeGenerator *fg, CallType callType) : fg_(fg), callType_(callType) {}

    /// @param callee The callee on which the method shall be called. Becomes the (this) context of the function.
    ///               If the method does not have a context, pass `nullptr`.
    /// @param type The type of the callee. Important to determine whether dispatch can immediately take place or
    ///             dynamic protocol conformance search has to be performed.
    /// @param astArgs Arguments from the AST, generated and passed to the function in order of appearance.
    ///                Additionaly arguments can be passed with supplArgs.
    /// @param function A prototype of the Function that shall be called. Unless this is a static dispatch, this value
    ///                 is only used to determine promises etc. but is not directly called.
    /// @param errorPointer The error pointer that should be passed to the function as last argument. Pass `nullptr`
    ///                     if no error pointer is required.
    /// @param supplArgs Any values provided are passed to the function after the values from `astArgs` but before
    ///                  `errorPointer`.
    llvm::Value* generate(llvm::Value *callee, const Type &type, const ASTArguments &astArgs,
                          Function *function, llvm::Value *errorPointer,
                          const std::vector<llvm::Value *> &supplArgs = {});

protected:
    std::vector<llvm::Value *> createArgsVector(llvm::Value *callee, const ASTArguments &args,
                                                llvm::Value *errorPointer,
                                                const std::vector<llvm::Value *> &supplArgs) const;
    FunctionCodeGenerator* fg() const { return fg_; }
    llvm::Value *createDynamicProtocolDispatch(Function *function, std::vector<llvm::Value *> args,
                                               const std::vector<Type> &genericArgs,
                                               llvm::Value *conformance);
    llvm::Value* buildFindProtocolConformance(const std::vector<llvm::Value *> &args, const Type &protocol);
private:
    llvm::Value *createDynamicDispatch(Function *function, const std::vector<llvm::Value *> &args,
                                       const std::vector<Type> &genericArgs);
    llvm::Value *dispatchFromVirtualTable(Function *function, llvm::Value *virtualTable,
                                              const std::vector<llvm::Value *> &args,
                                              const std::vector<Type> &genericArguments);
    FunctionCodeGenerator *fg_;
    CallType callType_;

    llvm::Value *getProtocolCallee(std::vector<llvm::Value *> &args, llvm::Value *conformance) const;
};

class MultiprotocolCallCodeGenerator : protected CallCodeGenerator {
public:
    using CallCodeGenerator::CallCodeGenerator;
    llvm::Value* generate(llvm::Value *callee, const Type &calleeType, const ASTArguments &args,
                          Function *function, llvm::Value *errorPointer, size_t multiprotocolN);
};

}  // namespace EmojicodeCompiler

#endif /* CallCodeGenerator_hpp */
