//
//  ProtocolFunction.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 14/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ProtocolFunction_hpp
#define ProtocolFunction_hpp

#include "Function.hpp"

namespace EmojicodeCompiler {

/// ProtocolFunction instances are the methods that are added to Protocols. ProtocolFunction instance are never
/// assigned a llvm::Function.
class ProtocolFunction : public Function {
public:
    using Function::Function;

    llvm::FunctionType* llvmFunctionType() const override { return llvmFunctionType_; }
    void setLlvmFunctionType(llvm::FunctionType *type) { llvmFunctionType_ = type; }
private:
    llvm::FunctionType* llvmFunctionType_;
};

} // namespace EmojicodeCompiler

#endif /* ProtocolFunction_hpp */
