//
//  ThunkBuilder.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 14/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ThunkBuilder_hpp
#define ThunkBuilder_hpp

#include <memory>

namespace EmojicodeCompiler {

class Function;
struct SourcePosition;
class Type;
class TypeExpectation;
class TypeContext;
class Package;
class Initializer;
class Class;

std::unique_ptr<Function> buildBoxingThunk(const TypeContext &declarator, const Function *method,
                                           const Function *methodImplementation);
std::unique_ptr<Function> buildCallableThunk(const TypeExpectation &expectation, const Type &destCallable,
                                           Package *pkg, const SourcePosition &p);
std::unique_ptr<Function> buildRequiredInitThunk(Class *klass, const Initializer *init);

} // namespace EmojicodeCompiler

#endif /* ThunkBuilder_hpp */

