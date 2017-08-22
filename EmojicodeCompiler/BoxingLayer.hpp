//
//  BoxingLayer.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 19/02/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef BoxingLayer_hpp
#define BoxingLayer_hpp

#include <utility>

#include "EmojicodeCompiler.hpp"
#include "Function.hpp"
#include "FunctionType.hpp"
#include "Generation/STIProvider.hpp"
#include "Types/Type.hpp"

namespace EmojicodeCompiler {

class BoxingLayer : public Function {
public:
    /// Creates a boxing layer for a protocol function.
    /// @parameter destinationFunction That function that should be called by the boxing layer. The "actual" method.
    BoxingLayer(Function *destinationFunction, EmojicodeString protocolName,
                const std::vector<Argument> &arguments, const Type &returnType, const SourcePosition &p)
    : Function(destinationFunction->protocolBoxingLayerName(std::move(protocolName)), AccessLevel::Private, true,
               destinationFunction->owningType(), destinationFunction->package(), p, false, EmojicodeString(), false,
               false, FunctionType::BoxingLayer), destinationReturnType_(destinationFunction->returnType),
        destinationFunction_(destinationFunction) {
        this->arguments = arguments;
        this->returnType = returnType;

        destinationArgumentTypes_.reserve(destinationFunction->arguments.size());
        for (auto &arg : destinationFunction->arguments) {
            destinationArgumentTypes_.emplace_back(arg.type);
        }                                                                                           
    }
    /// Creates a boxing layer for a callable. The argument/return conversions will be performed and
    /// INS_EXECUTE_CALLABLE callable will be applied to the this context.
    BoxingLayer(std::vector<Type> destinationArgumentTypes, Type destinationReturnType, Package *pkg,
                const std::vector<Argument> &arguments, const Type &returnType, const SourcePosition &p)
    : Function(EmojicodeString(), AccessLevel::Private, true,
               Type::callableIncomplete(), pkg, p, false, EmojicodeString(), false, false,
               FunctionType::BoxingLayer), destinationArgumentTypes_(std::move(destinationArgumentTypes)),
      destinationReturnType_(std::move(destinationReturnType)) {
          setVtiProvider(&STIProvider::globalStiProvider);
        this->returnType = returnType;
        this->arguments = arguments;
    }

    ContextType contextType() const override {
        if (owningType().type() == TypeType::Callable) {
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
