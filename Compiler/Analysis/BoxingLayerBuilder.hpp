//
//  BoxingLayerBuilder.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 14/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef BoxingLayerBuilder_hpp
#define BoxingLayerBuilder_hpp

#include <memory>

namespace EmojicodeCompiler {

class Function;
struct SourcePosition;
class Type;
class TypeExpectation;
class TypeContext;
class Package;

std::unique_ptr<Function> buildBoxingLayer(const TypeContext &declarator, const Function *method,
                                           const Function *methodImplementation);
std::unique_ptr<Function> buildBoxingLayer(const TypeExpectation &expectation, const Type &destCallable,
                                           Package *pkg, const SourcePosition &p);

} // namespace EmojicodeCompiler

#endif /* BoxingLayerBuilder_hpp */

