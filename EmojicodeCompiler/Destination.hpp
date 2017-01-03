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
#include <experimental/optional>
#include "Type.hpp"
#include "Variable.hpp"

enum class DestinationType {
    MutableDestination, ImmutableDestination, Temporary, Unknown
};

class Destination {
public:
    Destination() : type_(DestinationType::Unknown) {}
    static Destination temporary() { return Destination(true, DestinationType::Temporary); }
    static Destination mutableDestination() { return Destination(false, DestinationType::MutableDestination); }
    static Destination immutableDestination() { return Destination(false, DestinationType::ImmutableDestination); }

    bool isReference() const { return reference_; }
    bool isTemporary() const { return type_ == DestinationType::Temporary; }
    bool isMutable() const { return type_ == DestinationType::MutableDestination; }
    void validateForValueType() const;
    void validateIfValueType(Type type) const;
    void mutateVariable(SourcePosition p) const { if (mutatedVariable_) mutatedVariable_->mutate(p); }
    void setMutatedVariable(Variable &var) { mutatedVariable_ = &var; }
private:
    Destination(bool ref, DestinationType type) : reference_(ref), type_(type) {}
    bool reference_;
    DestinationType type_;
    Variable *mutatedVariable_ = nullptr;
};

#endif /* Destination_hpp */
