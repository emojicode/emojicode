//
//  DeinitializerBuilder.hpp
//  runtime
//
//  Created by Theo Weidmann on 30.08.18.
//

#ifndef DeinitializerBuilder_hpp
#define DeinitializerBuilder_hpp

namespace EmojicodeCompiler {

class TypeDefinition;
class ValueType;

void buildDeinitializer(TypeDefinition *typeDef);
void buildCopyRetain(ValueType *typeDef);

}  // namespace EmojicodeCompiler

#endif /* DeinitializerBuilder_hpp */
