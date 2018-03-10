//
//  Function.h
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Function_hpp
#define Function_hpp

#include "Emojis.h"
#include "FunctionType.hpp"
#include "Types/Generic.hpp"
#include "Types/Type.hpp"
#include "Types/TypeContext.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace llvm {
class Function;
class FunctionType;
}  // namespace llvm

namespace EmojicodeCompiler {

class ASTBlock;

enum class AccessLevel {
    Public, Private, Protected
};

struct Argument {
    Argument(std::u32string n, Type t) : variableName(std::move(n)), type(std::move(t)) {}

    /// The name of the variable
    std::u32string variableName;
    /// The type
    Type type;
};

class FunctionReification {
public:
    llvm::Function *function;
    unsigned int vti() { return vti_; }
    void setVti(unsigned int vti) { vti_ = vti; }
    llvm::FunctionType* functionType();
    void setFunctionType(llvm::FunctionType *type) { functionType_ = type; }
private:
    llvm::FunctionType* functionType_;
    unsigned int vti_ = 0;
};

/** Functions are callables that belong to a class or value type as either method, type method or initializer. */
class Function : public Generic<Function, FunctionReification> {
public:
    Function() = delete;
    Function(std::u32string name, AccessLevel level, bool final, Type owningType, Package *package, SourcePosition p,
             bool overriding, std::u32string documentationToken, bool deprecated, bool mutating, bool imperative,
             FunctionType type) : position_(std::move(p)), name_(std::move(name)), final_(final),
                                  overriding_(overriding), deprecated_(deprecated), imperative_(imperative),
                                  mutating_(mutating), access_(level), owningType_(std::move(owningType)),
                                  package_(package), documentation_(std::move(documentationToken)),
                                  functionType_(type) {}

    std::u32string name() const { return name_; }

    std::u32string protocolBoxingLayerName(const std::u32string &protocolName) {
        return protocolName + name()[0];
    }

    bool isExternal() const { return external_; }
    const std::string& externalName() const { return externalName_; }
    void makeExternal() { external_ = true; }
    void setExternalName(const std::string &name) {
        external_ = true;
        externalName_ = name;
    }

    /** Whether the method was marked as final and can’t be overridden. */
    bool final() const { return final_; }
    /** Whether the method is intended to override a super method. */
    bool overriding() const { return overriding_; }
    /** Whether the method is deprecated. */
    bool deprecated() const { return deprecated_; }

    bool isImperative() const { return imperative_; }
    /// Whether the function mutates the callee. Only relevant for value type instance methods.
    bool mutating() const { return mutating_; }

    /** Returns the access level to this method. */
    AccessLevel accessLevel() const { return access_; }

    const std::vector<Argument>& arguments() const { return arguments_; }

    void setArguments(std::vector<Argument> arguments) { arguments_ = std::move(arguments); }

    const Type& returnType() const { return returnType_; }

    void setReturnType(const Type &returnType) { returnType_ = returnType; }

    /** Returns the position at which this callable was defined. */
    const SourcePosition& position() const { return position_; }

    /// The type of this function when used as value.
    Type type() const;

    /** Type to which this function belongs.
     This can be Nothingness if the function doesn’t belong to any type (e.g. 🏁). */
    const Type& owningType() const { return owningType_; }
    void setOwningType(const Type &type) { owningType_ = type; }

    const std::u32string& documentation() const { return documentation_; }

    /// The package in which the function was defined.
    /// This does not necessarily match the package of @c owningType.
    Package* package() const { return package_; }

    /// Use this method to make a function a *heir* of this function, i.e. the heir function is guaranteed to have the
    /// same VTI’s for its reifications as this one and to be reified for all requests this function was.
    void appointHeir(Function *f) { tablePlaceHeirs_.push_back(f); }

    TypeContext typeContext() {
        auto type = owningType();
        if (type.type() == TypeType::ValueType || type.type() == TypeType::Enum) {
            type.setReference();
        }
        if (functionType() == FunctionType::ClassMethod) {
            type.setMeta(true);
        }
        return TypeContext(type, this);
    }

    FunctionType functionType() const { return functionType_; }

    void setAst(const std::shared_ptr<ASTBlock> &ast) { ast_ = ast; }
    const std::shared_ptr<ASTBlock>& ast() const { return ast_; }

    size_t variableCount() const { return variableCount_; }
    void setVariableCount(size_t variableCount) { variableCount_ = variableCount; }

    virtual ~Function() = default;

private:
    std::vector<Argument> arguments_;
    Type returnType_ = Type::noReturn();

    std::shared_ptr<ASTBlock> ast_;
    SourcePosition position_;
    std::u32string name_;
    std::vector<Function*> tablePlaceHeirs_;

    bool final_;
    bool overriding_;
    bool deprecated_;
    bool imperative_;

    const bool mutating_;
    bool external_ = false;

    std::string externalName_;
    AccessLevel access_;
    Type owningType_;
    Package *package_;
    std::u32string documentation_;
    FunctionType functionType_;
    size_t variableCount_ = 0;
};

}  // namespace EmojicodeCompiler

#endif /* Function_hpp */
