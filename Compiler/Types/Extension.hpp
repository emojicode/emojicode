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
    Extension(Type extendedType, Package *pkg, SourcePosition p, const std::u32string &documentation)
    : TypeDefinition(extendedType.typeDefinition()->name(), pkg, std::move(p), documentation, false),
    extendedType_(std::move(extendedType)) {}

    bool canBeUsedToResolve(TypeDefinition *resolutionConstraint) const override { return false; }

    void addInstanceVariable(const InstanceVariableDeclaration &declaration) override {
        throw CompilerError(position(), "An extension cannot add an instance variable.");
    }

    void extend();

    const Type& extendedType() { return extendedType_; }
private:
    Type extendedType_;
};

} // namespace EmojicodeCompiler

#endif /* Extension_hpp */
