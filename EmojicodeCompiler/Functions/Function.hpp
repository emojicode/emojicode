//
//  Function.h
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Function_hpp
#define Function_hpp

#include "../CompilerError.hpp"
#include "../Generation/FunctionWriter.hpp"
#include "../Types/Class.hpp"
#include "../Types/Generic.hpp"
#include "../Types/Type.hpp"
#include "FunctionType.hpp"
#include <algorithm>
#include <experimental/optional>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <utility>
#include <vector>

namespace EmojicodeCompiler {

class VTIProvider;
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

/** Functions are callables that belong to a class or value type as either method, type method or initializer. */
class Function : public Generic<Function> {
    friend void Class::prepareForCG();
    friend Protocol;
public:
    Function(std::u32string name, AccessLevel level, bool final, Type owningType, Package *package, SourcePosition p,
             bool overriding, std::u32string documentationToken, bool deprecated, bool mutating, FunctionType type)
    : position_(std::move(p)), name_(std::move(name)), final_(final), overriding_(overriding), deprecated_(deprecated), mutating_(mutating),
    access_(level), owningType_(std::move(owningType)), package_(package), documentation_(std::move(documentationToken)),
    functionType_(type) {}

    std::u32string name() const { return name_; }

    std::u32string protocolBoxingLayerName(const std::u32string &protocolName) {
        return protocolName + name()[0];
    }

    /** Whether the method is implemented natively and Run-Time Native Linking must occur. */
    bool isNative() const { return linkingTableIndex_ > 0; }
    unsigned int linkingTabelIndex() const { return linkingTableIndex_; }
    void setLinkingTableIndex(int index);
    /** Whether the method was marked as final and can’t be overriden. */
    bool final() const { return final_; }
    /** Whether the method is intended to override a super method. */
    bool overriding() const { return overriding_; }
    /** Whether the method is deprecated. */
    bool deprecated() const { return deprecated_; }
    /** Returns the access level to this method. */
    AccessLevel accessLevel() const { return access_; }

    std::vector<Argument> arguments;
    /** Return type of the method */
    Type returnType = Type::nothingness();

    /** Returns the position at which this callable was defined. */
    const SourcePosition& position() const { return position_; }

    /// The type of this function when used as value.
    Type type() const;

    /** Type to which this function belongs.
     This can be Nothingness if the function doesn’t belong to any type (e.g. 🏁). */
    Type owningType() const { return owningType_; }
    void setOwningType(const Type &type) { owningType_ = type; }

    const std::u32string& documentation() const { return documentation_; }

    /// The package in which the function was defined.
    /// This does not necessarily match the package of @c owningType.
    Package* package() const { return package_; }

    /// Issues a warning at the given position if the function is deprecated.
    void deprecatedWarning(const SourcePosition &p) const;

    /// Checks that no promises were broken and applies boxing if necessary.
    /// @returns false iff a value for protocol was given and the arguments or the return type are storage incompatible.
    /// This indicates that a BoxingLayer must be created.
    bool enforcePromises(Function *super, const TypeContext &typeContext, const Type &superSource,
                         std::experimental::optional<TypeContext> protocol);

    void registerOverrider(Function *f) { overriders_.push_back(f); }

    /// Returns the VTI for this function. If the function is yet to be assigned a VTI, a VTI is obtained from the
    /// VTI provider. If the function was not marked used, it will be.
    /// This function is a shortcut to calling @c assignVti and @c setUsed and then getting the VTI.
    /// @warning This method must only be called if the function will be needed at run-time and
    /// should be assigned a VTI.
    int vtiForUse();

    /** Sets the @c VTIProvider which should be used to assign this method a VTI and to update the VTI counter. */
    void setVtiProvider(VTIProvider *provider);
    /// Sets the VTI to the given value.
    void setVti(int vti);
    /// Returns the VTI this function was assigned.
    /// @throws std::logic_error if the function wasn’t assigned a VTI
    int getVti() const;
    /// Marks this function as used. Propagates to all overriders.
    virtual void setUsed(bool enqueue = true);
    /// @returns Whether the function was used.
    bool used() const { return used_; }
    /// Assigns this method a VTI without marking it as used. Propagates to all overriders.
    virtual void assignVti();
    /// @returns Whether the function was assigned a VTI.
    virtual bool assigned() const;

    TypeContext typeContext() {
        auto type = owningType();
        if (type.type() == TypeType::ValueType || type.type() == TypeType::Enum) {
            type.setReference();
        }
        return TypeContext(type, this);
    }

    /// Whether the function mutates the callee. Only relevant for value type instance methods.
    bool mutating() const { return mutating_; }

    FunctionType functionType() const { return functionType_; }
    void setFunctionType(FunctionType type) { functionType_ = type; }

    virtual ContextType contextType() const {
        switch (functionType()) {
            case FunctionType::ObjectMethod:
            case FunctionType::ObjectInitializer:
                return ContextType::Object;
                break;
            case FunctionType::ValueTypeMethod:
            case FunctionType::ValueTypeInitializer:
                return ContextType::ValueReference;
                break;
            case FunctionType::ClassMethod:
            case FunctionType::Function:
                return ContextType::None;
                break;
            case FunctionType::BoxingLayer:
                throw std::logic_error("contextType for BoxingLayer called on Function class");
        }
    }

    void setAst(const std::shared_ptr<ASTBlock> &ast) { ast_ = ast; }
    const std::shared_ptr<ASTBlock>& ast() const { return ast_; }

    int fullSize() const { return fullSize_; }
    void setFullSize(int c) { fullSize_ = c; }

    FunctionWriter writer_;
    std::vector<FunctionObjectVariableInformation>& objectVariableInformation() { return objectVariableInformation_; }
    size_t variableCount() const { return variableCount_; }
    void setVariableCount(size_t variableCount) { variableCount_ = variableCount; }
protected:
    std::vector<Function*> overriders_;
    bool used_ = false;
private:
    std::shared_ptr<ASTBlock> ast_;
    SourcePosition position_;
    std::u32string name_;
    int vti_ = -1;
    bool final_;
    bool overriding_;
    bool deprecated_;
    bool mutating_;
    unsigned int linkingTableIndex_ = 0;
    AccessLevel access_;
    Type owningType_;
    Package *package_;
    std::u32string documentation_;
    VTIProvider *vtiProvider_ = nullptr;
    FunctionType functionType_;
    size_t variableCount_ = 0;

    std::vector<FunctionObjectVariableInformation> objectVariableInformation_;
    int fullSize_ = -1;
};

}  // namespace EmojicodeCompiler

#endif /* Function_hpp */
