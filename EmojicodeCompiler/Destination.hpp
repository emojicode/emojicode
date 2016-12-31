//
//  Destination.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 30/12/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Destination_hpp
#define Destination_hpp

#include <stdexcept>
#include "Type.hpp"

enum class DestinationType {
    WithDestination, Temporary, Unknown
};

class Destination {
public:
    Destination() : type_(DestinationType::Unknown) {}
    static Destination temporary() { return Destination(true, DestinationType::Temporary); }
    static Destination with() { return Destination(false, DestinationType::WithDestination); }

    bool isReference() const { return reference_; }
    bool isTemporary() const { return type_ == DestinationType::Temporary; }
    void validateForValueType() const;
    void validateIfValueType(Type type) const;
private:
    Destination(bool ref, DestinationType type) : reference_(ref), type_(type) {}
    bool reference_;
    DestinationType type_;
};

#endif /* Destination_hpp */
