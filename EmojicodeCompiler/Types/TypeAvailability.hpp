//
//  TypeAvailability.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 13/04/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef TypeAvailability_hpp
#define TypeAvailability_hpp

namespace EmojicodeCompiler {

struct SourcePosition;

enum class TypeAvailability {
    /** The type is known at compile type, can’t change and will be available at runtime. Instruction to retrieve the
     type at runtime were written to the file. */
    StaticAndAvailabale,
    /** The type is known at compile type, but a another compatible type might be provided at runtime instead.
     The type will be available at runtime. Instruction to retrieve the type at runtime were written to the file. */
    DynamicAndAvailable,
    /** The type is fully known at compile type but won’t be available at runtime (Enum, ValueType etc.).
     Nothing was written to the file if this value is returned. */
    StaticAndUnavailable,
};

void notStaticError(TypeAvailability t, const SourcePosition &p, const char *name);

inline bool isStatic(TypeAvailability t) {
    return t == TypeAvailability::StaticAndUnavailable || t == TypeAvailability::StaticAndAvailabale;
}

}  // namespace EmojicodeCompiler

#endif /* TypeAvailability_hpp */
