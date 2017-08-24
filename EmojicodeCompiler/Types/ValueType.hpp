//
//  ValueType.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 12/06/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef ValueType_hpp
#define ValueType_hpp

#include "../Generation/STIProvider.hpp"
#include "TypeDefinition.hpp"
#include <utility>
#include <vector>

namespace EmojicodeCompiler {

class ValueType : public TypeDefinition {
public:
    static const std::vector<ValueType *>& valueTypes() { return valueTypes_; }

    ValueType(std::u32string name, Package *p, SourcePosition pos, const std::u32string &documentation)
        : TypeDefinition(std::move(name), p, std::move(pos), documentation) {
        valueTypes_.push_back(this);
    }

    void prepareForSemanticAnalysis() override;

    void prepareForCG() override;
    int usedFunctionCount() const { return vtiProvider_.usedCount(); };

    int size() const override { return primitive_ ? 1 : TypeDefinition::size(); }

    void addMethod(Function *method) override;
    void addInitializer(Initializer *initializer) override;
    void addTypeMethod(Function *method) override;

    bool canBeUsedToResolve(TypeDefinition *resolutionConstraint) const override {
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
//        for (auto variable : instanceScope().map()) {
//            variable.second.type().objectVariableRecords(variable.second.id(), &boxObjectVariableInformation_.back());
//        }
        // TODO: fix
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

    VTIProvider *protocolMethodVtiProvider() override {
        return &vtiProvider_;
    }

    static std::vector<std::vector<ObjectVariableInformation>> boxObjectVariableInformation_;
    std::map<std::vector<Type>, uint32_t> genericIds_;
};

}  // namespace EmojicodeCompiler

#endif /* ValueType_hpp */
