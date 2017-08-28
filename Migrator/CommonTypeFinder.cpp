//
//  CommonTypeFinder.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "CommonTypeFinder.hpp"
#include "TypeContext.hpp"
#include "TypeDefinitionFunctional.hpp"
#include <algorithm>

namespace EmojicodeCompiler {

void CommonTypeFinder::addType(const Type &type, const TypeContext &typeContext) {
    if (!firstTypeFound_) {
        commonType_ = type;
        firstTypeFound_ = true;
        if (type.canHaveProtocol()) {
            commonProtocols_ = type.typeDefinitionFunctional()->protocols();
        }
        return;
    }

    if (!type.compatibleTo(commonType_, typeContext)) {
        if (commonType_.compatibleTo(type, typeContext)) {
            commonType_ = type;
        }
        else if (type.type() == TypeContent::Class && commonType_.type() == TypeContent::Class) {
            commonType_ = Type::someobject();
        }
        else {
            commonType_ = Type::something();
        }
    }

    if (!commonProtocols_.empty()) {
        if (!type.canHaveProtocol()) {
            commonProtocols_.clear();
            return;
        }

        auto &protocols = type.typeDefinitionFunctional()->protocols();
        std::vector<Type> newCommonProtocols;
        for (auto &protocol : protocols) {
            auto b = std::any_of(commonProtocols_.begin(), commonProtocols_.end(), [&protocol, &typeContext](const Type &p) {
                return protocol.identicalTo(p, typeContext, nullptr);
            });
            if (b) {
                newCommonProtocols.push_back(protocol);
            }
        }
        commonProtocols_ = newCommonProtocols;
    }
}

Type CommonTypeFinder::getCommonType(const SourcePosition &p) const {
    if (!firstTypeFound_) {
        compilerWarning(p, "Type is ambigious without more context.");
    }
    else if (commonType_.type() == TypeContent::Something || commonType_.type() == TypeContent::Someobject) {
        if (commonProtocols_.size() > 1) {
            return Type(commonProtocols_, false);
        }
        if (commonProtocols_.size() == 1) {
            return commonProtocols_.front();
        }
        compilerWarning(p, "Common type was inferred to be %s.",
                        commonType_.toString(Type::nothingness(), true).c_str());
    }
    return commonType_;
}

}  // namespace EmojicodeCompiler
