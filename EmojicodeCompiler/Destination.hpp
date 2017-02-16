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
#include "StorageType.hpp"

enum class DestinationMutability {
    Mutable, Immutable, Unknown
};

class Destination {
public:
    Destination(DestinationMutability mutability, StorageType type) : mutability_(mutability), type_(type) {}
    static Destination temporaryReference(StorageType type = StorageType::SimpleIfPossible) {
        return Destination(true, type);
    }

    bool isTemporaryReference() const { return temporary_; }
    bool isMutable() const { return mutability_ == DestinationMutability::Mutable; }

    StorageType storageType() const { return type_; }

    void mutateVariable(SourcePosition p) const { if (mutatedVariable_) mutatedVariable_->mutate(p); }
    void setMutatedVariable(Variable &var) { mutatedVariable_ = &var; }

    /// Returns the storage type to which the value of the given type should be unboxed, if the destination expects
    /// the most unboxed value as indicated by a storage type of @c StorageType::SimpleIfPossible.
    /// @returns The storage type to which the value can be unboxed or the original storage type if the value should
    /// not be unboxed.
    StorageType simplify(Type type) const {
        if (storageType() == StorageType::SimpleIfPossible) {
            if (type.requiresBox()) {
                return StorageType::Box;
            }
            else {
                return type.optional() ? StorageType::SimpleOptional : StorageType::Simple;
            }
        }
        return storageType();
    }
private:
    Destination(bool temp, StorageType type)
        : temporary_(temp), mutability_(DestinationMutability::Unknown), type_(type) {}
    bool temporary_ = false;
    DestinationMutability mutability_;
    StorageType type_;
    Variable *mutatedVariable_ = nullptr;
};

#endif /* Destination_hpp */
