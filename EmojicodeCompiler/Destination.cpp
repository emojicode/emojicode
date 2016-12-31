//
//  Destination.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 30/12/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Destination.hpp"
#include "ValueType.hpp"

void Destination::validateForValueType() const {
    if (type_ == DestinationType::Unknown) {
        throw std::logic_error("Instantiating value type without proper destination");
    }
    if (isTemporary() && !isReference()) {
        throw std::logic_error("Instantiating temporary value type but no reference expected");
    }
}

void Destination::validateIfValueType(Type type) const {
    if (type.type() == TypeContent::ValueType && !type.valueType()->isPrimitive()) {
        validateForValueType();
    }
}
