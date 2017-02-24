//
//  CommonTypeFinder.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "CommonTypeFinder.hpp"
#include <algorithm>
#include "TypeContext.hpp"
#include "TypeDefinitionFunctional.hpp"

void CommonTypeFinder::addType(Type t, TypeContext typeContext) {
    if (!firstTypeFound_) {
        commonType_ = t;
        firstTypeFound_ = true;
        if (t.canHaveProtocol()) commonProtocols_ = t.typeDefinitionFunctional()->protocols();
        return;
    }

    if (!t.compatibleTo(commonType_, typeContext)) {
        if (commonType_.compatibleTo(t, typeContext)) {
            commonType_ = t;
        }
        else if (t.type() == TypeContent::Class && commonType_.type() == TypeContent::Class) {
            commonType_ = Type::someobject();
        }
        else {
            commonType_ = Type::something();
        }
    }

    if (commonProtocols_.size() > 0) {
        if (!t.canHaveProtocol()) {
            commonProtocols_.clear();
            return;
        }

        auto &protocols = t.typeDefinitionFunctional()->protocols();
        std::vector<Type> newCommonProtocols;
        for (auto protocol : protocols) {
            auto found = std::find_if(commonProtocols_.begin(), commonProtocols_.end(),
                                      [protocol, typeContext](const Type &p) {
                return protocol.identicalTo(p, typeContext, nullptr);
            });
            if (found != commonProtocols_.end()) {
                newCommonProtocols.push_back(protocol);
            }
        }
        commonProtocols_ = newCommonProtocols;
    }
}

Type CommonTypeFinder::getCommonType(SourcePosition p) const {
    if (!firstTypeFound_) {
        compilerWarning(p, "Type is ambigious without more context.");
    }
    else if (commonType_.type() == TypeContent::Something || commonType_.type() == TypeContent::Someobject) {
        if (commonProtocols_.size() > 1) {
            return Type(commonProtocols_, false);
        }
        else if (commonProtocols_.size() == 1) {
            return commonProtocols_.front();
        }
        else {
            compilerWarning(p, "Common type was inferred to be %s.",
                            commonType_.toString(Type::nothingness(), true).c_str());
        }
    }
    return commonType_;
}
