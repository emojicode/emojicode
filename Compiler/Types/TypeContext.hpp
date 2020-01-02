//
//  TypeContext.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef TypeContext_hpp
#define TypeContext_hpp

#include "Type.hpp"
#include <vector>

namespace EmojicodeCompiler {

class Function;

/// A TypeContext instance represents the context that is used to determine the meaning of a parsed type identifier.
/// For instance, the meaning of the generic type variable A can only be determined with a type context.
///
/// A TypeContext instance can have a calleeType, this is the type in whose context code is to be interpreted. E.g.
/// in a class method the calleeType is the class, in a protocol conformance declaration in a value type the calleeType
/// is the value type as protocol conformance declarations allow generic type variables that refer to the type being
/// defined.
/// TypeContext instance can also have a function. The function refers to the function in whose context the code lies.
/// E.g. in a class method, the function would, of course, be the class method. In an instance variable declaration the
/// the TypeContext doesn't have a function, as the code is not in the context of a function.
///
/// Furthermore, a TypeContext instance can have functionGenericArguments. Those are the generic arguments provided
/// to a function call and are only set when a function called is analysed. Type::resolveOn() uses these to resolve
/// the local generic arguments.
class TypeContext {
public:
    /// Constructs a TypeContext with neither a calleeType nor a function.
    TypeContext() : calleeType_(Type::noReturn()) {}
    /// Constructs a TypeContext with only a calleeType
    explicit TypeContext(Type callee) : calleeType_(std::move(callee)) {}
    /// Constructs a TypeContext with a calleeType and a function.
    TypeContext(Type callee, Function *p) : calleeType_(std::move(callee)), function_(p) {}
    TypeContext(Type callee, Function *p, const std::vector<Type> *args)
        : calleeType_(std::move(callee)), function_(p), functionGenericArguments_(args) {}

    const Type& calleeType() const { return calleeType_; }
    /// Returns the function in whose context the code is.
    /// @returns The function with which this TypeContext was constructed or @c nullptr if no function is provided.
    Function* function() const { return function_; }
    const std::vector<Type>* functionGenericArguments() const { return functionGenericArguments_; }
private:
    Type calleeType_;
    Function *function_ = nullptr;
    const std::vector<Type> *functionGenericArguments_ = nullptr;
};

}  // namespace EmojicodeCompiler

#endif /* TypeContext_hpp */
