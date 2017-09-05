//
//  ValueType.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 12/06/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef ValueType_hpp
#define ValueType_hpp

#include "../Application.hpp"
#include "../Package/Package.hpp"
#include "TypeDefinition.hpp"
#include <utility>
#include <vector>

namespace EmojicodeCompiler {

class ValueType : public TypeDefinition {
public:
    ValueType(std::u32string name, Package *p, SourcePosition pos, const std::u32string &documentation)
        : TypeDefinition(std::move(name), p, std::move(pos), documentation) {}

    void prepareForSemanticAnalysis() override;

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
        auto id = package()->app()->boxObjectVariableInformation().size();
        genericIds_.emplace(genericArguments, id);
        package()->app()->boxObjectVariableInformation().emplace_back();

//        for (auto variable : instanceScope().map()) {
//            variable.second.type().objectVariableRecords(variable.second.id(), &boxObjectVariableInformation_.back());
//        }
        // TODO: fix
        return static_cast<uint32_t>(id);
    }
    
    const std::map<std::vector<Type>, uint32_t>& genericIds() { return genericIds_; }

    void makePrimitive() { primitive_ = true; }
    bool isPrimitive() { return primitive_; }
private:
    bool primitive_ = false;
    std::map<std::vector<Type>, uint32_t> genericIds_;
};

}  // namespace EmojicodeCompiler

#endif /* ValueType_hpp */
