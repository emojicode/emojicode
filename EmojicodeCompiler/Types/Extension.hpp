//
//  Extension.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 06/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Extension_hpp
#define Extension_hpp

#include "TypeDefinition.hpp"

namespace EmojicodeCompiler {

class Extension : public TypeDefinition {
public:
    Extension(Type extendedType, Package *pkg, SourcePosition p, const EmojicodeString &documentation)
    : TypeDefinition(extendedType.typeDefinition()->name(), pkg, std::move(p), documentation),
    extendedType_(std::move(extendedType)) {}

    bool canBeUsedToResolve(TypeDefinition *resolutionConstraint) const override { return false; }

    void addInstanceVariable(const InstanceVariableDeclaration&) override {
        throw CompilerError(position(), "An extension cannot add an instance variable to a type from another package.");
    }

    void prepareForSemanticAnalysis() override { extend(); }
    void extend();

    const Type& extendedType() { return extendedType_; }
private:
    Type extendedType_;
    VTIProvider* protocolMethodVtiProvider() override { return nullptr; }
};

} // namespace EmojicodeCompiler

#endif /* Extension_hpp */
