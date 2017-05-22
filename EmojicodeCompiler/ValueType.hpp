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
#include "BoxIDProvider.hpp"
#include <vector>

class ValueType : public TypeDefinitionFunctional, public BoxIDProvider {
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

    void makePrimitive() { primitive_ = true; }
    bool isPrimitive() { return primitive_; }
private:
    static std::vector<ValueType *> valueTypes_;
    ValueTypeVTIProvider vtiProvider_;
    bool primitive_ = false;
};

#endif /* ValueType_hpp */
