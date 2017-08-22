//
//  TypeExpectation.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 12/03/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef TypeExpectation_hpp
#define TypeExpectation_hpp

#include "Type.hpp"
#include "../Scoping/Variable.hpp"

namespace EmojicodeCompiler {

enum class ExpectationMode {
    /// The value must be (un)boxed to match the expectation’s storage type exactly.
    Convert,
    /// The value should be unboxed if possible. The expectation’s storage type is ignored.
    Simplify,
    /// No conversion or simplification may take place.
    NoAction,
};

class TypeExpectation : public Type {
public:
    /// Creates a type expectation from the given type. The mode is set to "convert".
    explicit TypeExpectation(const Type &type) : Type(type), expectationMode_(ExpectationMode::Convert) {}
    /// Creates a type expectation with the given parameteres that expects the value to be simplified as far as possible
    TypeExpectation(bool isReference, bool isMutable)
        : Type(isReference, false, isMutable), expectationMode_(ExpectationMode::Simplify) {}
    TypeExpectation(bool isReference, bool forceBox, bool isMutable)
        : Type(isReference, forceBox, isMutable), expectationMode_(ExpectationMode::Convert) {}
    /// Creates a type expectation with mode "no action".
    TypeExpectation() : Type(false, false, false), expectationMode_(ExpectationMode::NoAction) {}

    /// Returns the storage type to which the value of the given type should be unboxed, if the destination expects
    /// the most unboxed value as indicated by a storage type of @c StorageType::SimpleIfPossible.
    /// @returns The storage type to which the value can be unboxed or the original storage type if the value should
    /// not be unboxed.
    StorageType simplifyType(Type type) const {
        if (expectationMode_ == ExpectationMode::Simplify) {
            if (type.requiresBox()) {
                return StorageType::Box;
            }
            else {
                return type.optional() ? StorageType::SimpleOptional : StorageType::Simple;
            }
        }
        return storageType();
    }

    bool shouldPerformBoxing() const { return expectationMode_ != ExpectationMode::NoAction; }
private:
    ExpectationMode expectationMode_ = ExpectationMode::NoAction;
};

}  // namespace EmojicodeCompiler

#endif /* TypeExpectation_hpp */
