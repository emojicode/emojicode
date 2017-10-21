//
//  ValueType.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 12/06/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef ValueType_hpp
#define ValueType_hpp

#include "TypeDefinition.hpp"
#include <utility>
#include <vector>

namespace llvm {
class GlobalVariable;
}  // namespace llvm

namespace EmojicodeCompiler {

class ValueType : public TypeDefinition {
public:
    ValueType(std::u32string name, Package *p, SourcePosition pos, const std::u32string &documentation, bool exported)
        : TypeDefinition(std::move(name), p, std::move(pos), documentation, exported) {}

    void prepareForSemanticAnalysis() override;

    bool canBeUsedToResolve(TypeDefinition *resolutionConstraint) const override {
        return resolutionConstraint == this;
    }

    llvm::GlobalVariable* valueTypeMetaFor(const std::vector<Type> &genericArguments);
    void addValueTypeMetaFor(const std::vector<Type> &genericArguments, llvm::GlobalVariable *valueTypeMeta);

    void makePrimitive() { primitive_ = true; }
    bool isPrimitive() { return primitive_; }
private:
    bool primitive_ = false;
    std::map<std::vector<Type>, llvm::GlobalVariable*> valueTypeMetas_;
};

}  // namespace EmojicodeCompiler

#endif /* ValueType_hpp */
