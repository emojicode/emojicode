//
//  BoxingLayer.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 19/02/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef BoxingLayer_hpp
#define BoxingLayer_hpp

#include "EmojicodeCompiler.hpp"
#include "Type.hpp"
#include "Function.hpp"

namespace EmojicodeCompiler {

class BoxingLayer : public Function {
public:
    /// Creates a boxing layer for a protocol function.
    /// @parameter destinationFunction That function that should be called by the boxing layer. The "actual" method.
    BoxingLayer(Function *destinationFunction, EmojicodeString protocolName, VTIProvider *provider,
                const std::vector<Argument> &arguments, const Type &returnType, const SourcePosition &p)
    : Function(destinationFunction->protocolBoxingLayerName(protocolName), AccessLevel::Private, true,
               destinationFunction->owningType(), destinationFunction->package(), p, false, EmojicodeString(), false,
               false, FunctionPAGMode::BoxingLayer), destinationReturnType_(destinationFunction->returnType),
        destinationFunction_(destinationFunction) {
        setVtiProvider(provider);
        this->arguments = arguments;
        this->returnType = returnType;

        destinationArgumentTypes_.reserve(destinationFunction->arguments.size());
        for (auto &arg : destinationFunction->arguments) {
            destinationArgumentTypes_.emplace_back(arg.type);
        }
    }
    /// Creates a boxing layer for a callable. The argument/return conversions will be performed and
    /// INS_EXECUTE_CALLABLE callable will be applied to the this context.
    BoxingLayer(const std::vector<Type> &destinationArgumentTypes, const Type &destinationReturnType, Package *pkg,
                const std::vector<Argument> &arguments, const Type &returnType, const SourcePosition &p)
    : Function(EmojicodeString(), AccessLevel::Private, true,
               Type::callableIncomplete(), pkg, p, false, EmojicodeString(), false, false,
               FunctionPAGMode::BoxingLayer), destinationArgumentTypes_(destinationArgumentTypes),
      destinationReturnType_(destinationReturnType) {
        setVtiProvider(&Function::pureFunctionsProvider);
        this->returnType = returnType;
        this->arguments = arguments;
    }

    ContextType contextType() const override {
        if (owningType().type() == TypeContent::Callable) {
            return ContextType::Object;
        }
        return destinationFunction()->contextType();
    }

    const std::vector<Type>& destinationArgumentTypes() const { return destinationArgumentTypes_; }
    const Type& destinationReturnType() const { return destinationReturnType_; }
    /// Returns the function whill will be called. Returns @c nullptr if this is a callable boxing layer.
    Function* destinationFunction() const { return destinationFunction_; }
private:
    std::vector<Type> destinationArgumentTypes_;
    Type destinationReturnType_;
    Function *destinationFunction_ = nullptr;
};

}  // namespace EmojicodeCompiler

#endif /* BoxingLayer_hpp */
