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
#include "Function.hpp"

class BoxingLayer : public Function {
public:
    BoxingLayer(Function *function, EmojicodeString protocolName, VTIProvider *provider, const Type &preturnType,
                SourcePosition p)
    : Function(function->protocolBoxingLayerName(protocolName), AccessLevel::Private, true,
               function->owningType(), function->package(), p, false, EmojicodeString(), false, false,
               FunctionPAGMode::BoxingLayer), destinationFunction_(function) {
        setVtiProvider(provider);
        returnType = preturnType;
    }
    Function* destinationFunction() const { return destinationFunction_; }
private:
    Function *destinationFunction_;
};

#endif /* BoxingLayer_hpp */
