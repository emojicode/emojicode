//
//  FunctionPAGInterface.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 11/04/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef FunctionPAGInterface_hpp
#define FunctionPAGInterface_hpp

#include "FunctionWriter.hpp"
#include "TokenStream.hpp"
#include "CommonTypeFinder.hpp"
#include "TypeAvailability.hpp"
#include "FunctionPAGMode.hpp"
#include "CallableScoper.hpp"

class FunctionPAGInterface {
public:
    virtual FunctionWriter& writer() = 0;
    virtual TokenStream& stream() = 0;
    virtual Package* package() = 0;
    virtual FunctionPAGMode mode() const = 0;
    virtual const TypeContext& typeContext() = 0;
    virtual void box(const TypeExpectation &expectation, Type &returnType, WriteLocation location) const = 0;
    virtual void box(const TypeExpectation &expectation, Type returnType) const = 0;
    virtual Type parseFunctionCall(const Type &type, Function *p, const Token &token) = 0;
    /// Attempts to parse an expression.
    /// @param expectation The expectation. The value is only modified if a variable is parsed.
    /// @throws CompilerError if a compiler error occurs.
    /// @returns The type of the expression.
    virtual Type parseExpr(TypeExpectation &&expectation) = 0;
    /// Same as parseExpr but ensures that the expression type is compatible to @c type
    virtual Type parseTypeSafeExpr(Type type, std::vector<CommonTypeFinder>* = nullptr) = 0;
    /// Marks the current statement as effective
    virtual void makeEffective() = 0;
    /// Parses a type name and writes instructions to fetch it at runtime.
    /// @returns A pair containing the parsed type and the availability of the type.
    /// @throws CompilerError if parsing the type name fails.
    virtual std::pair<Type, TypeAvailability> parseTypeAsValue(const SourcePosition &p,  const TypeExpectation &expectation) = 0;
    /// Checks that the function @c function can be accessed from the function that is being compiled.
    /// @throws CompilerError if the function cannot be accessed.
    virtual void checkAccessLevel(Function *function, const SourcePosition &p) const = 0;

    virtual Type parseTypeDeclarative(const TypeContext &typeContext, TypeDynamism dynamism,
                                      Type expectation = Type::nothingness(), TypeDynamism *dynamicType = nullptr) = 0;

    /// Returns the scoper that is used to scope the function.
    virtual CallableScoper& scoper() = 0;
    virtual void popScope() = 0;
};

#endif /* FunctionPAGInterface_hpp */
