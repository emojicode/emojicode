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

class ValueType : public TypeDefinitionFunctional {
public:
    static const std::vector<ValueType *>& valueTypes() { return valueTypes_; }

    ValueType(EmojicodeString name, Package *p, SourcePosition pos, const EmojicodeString &documentation)
        : TypeDefinitionFunctional(name, p, pos, documentation) {
        valueTypes_.push_back(this);
    }

    bool canBeUsedToResolve(TypeDefinitionFunctional *resolutionConstraint) const override { return false; }

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

    void makePrimitive() { primitive_ = true; }
    bool isPrimitive() { return primitive_; }

    int boxIdentifier() const { return id_; }

    static int maxBoxIndetifier() { return nextId - 1; }
private:
    static int nextId;
    static std::vector<ValueType *> valueTypes_;
    ValueTypeVTIProvider vtiProvider_;
    bool primitive_ = false;
    int id_ = nextId++;
};

#endif /* ValueType_hpp */
