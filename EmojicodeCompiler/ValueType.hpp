//
//  ValueType.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 12/06/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef ValueType_hpp
#define ValueType_hpp

#include "TypeDefinitionFunctional.hpp"
#include "VTIProvider.hpp"
#include "Function.hpp"
#include <vector>

namespace EmojicodeCompiler {

class ValueType : public TypeDefinitionFunctional {
public:
    static const std::vector<ValueType *>& valueTypes() { return valueTypes_; }

    ValueType(EmojicodeString name, Package *p, SourcePosition pos, const EmojicodeString &documentation)
        : TypeDefinitionFunctional(name, p, pos, documentation) {
        valueTypes_.push_back(this);
    }

    void finalize() override;
    int usedFunctionCount() const { return vtiProvider_.usedCount(); };

    virtual int size() const override { return primitive_ ? 1 : TypeDefinitionFunctional::size(); }

    void addMethod(Function *method) override {
        TypeDefinitionFunctional::addMethod(method);
        method->package()->registerFunction(method);
    }
    void addInitializer(Initializer *initializer) override {
        TypeDefinitionFunctional::addInitializer(initializer);
        initializer->package()->registerFunction(initializer);
    }
    void addTypeMethod(Function *method) override {
        TypeDefinitionFunctional::addTypeMethod(method);
        method->package()->registerFunction(method);
    }

    bool canBeUsedToResolve(TypeDefinitionFunctional *resolutionConstraint) const override {
        return resolutionConstraint == this;
    }

    uint32_t boxIdFor(const std::vector<Type> &genericArguments) {
        auto genericId = genericIds_.find(genericArguments);
        if (genericId != genericIds_.end()) {
            return genericId->second;
        }
        auto id = boxObjectVariableInformation_.size();
        genericIds_.emplace(genericArguments, id);
        boxObjectVariableInformation_.emplace_back();
        for (auto variable : instanceScope().map()) {
            variable.second.type().objectVariableRecords(variable.second.id(), boxObjectVariableInformation_.back());
        }
        return static_cast<uint32_t>(id);
    }
    static uint32_t maxBoxIndetifier() { return static_cast<uint32_t>(boxObjectVariableInformation_.size()); }
    static const std::vector<std::vector<ObjectVariableInformation>>& boxObjectVariableInformation() {
        return boxObjectVariableInformation_;
    };
    const std::map<std::vector<Type>, uint32_t>& genericIds() { return genericIds_; }

    void makePrimitive() { primitive_ = true; }
    bool isPrimitive() { return primitive_; }
private:
    static std::vector<ValueType *> valueTypes_;
    ValueTypeVTIProvider vtiProvider_;
    bool primitive_ = false;

    static std::vector<std::vector<ObjectVariableInformation>> boxObjectVariableInformation_;
    std::map<std::vector<Type>, uint32_t> genericIds_;
};

}  // namespace EmojicodeCompiler

#endif /* ValueType_hpp */
