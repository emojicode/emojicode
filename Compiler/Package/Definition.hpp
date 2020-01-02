//
//  Definition.hpp
//  emojicodec
//
//  Created by Theo Weidmann on 25.12.19.
//

#ifndef Definition_hpp
#define Definition_hpp

#include "Lex/SourcePosition.hpp"
#include <string>

namespace EmojicodeCompiler {

class Package;

/// Superclass of all definitions, i.e. types and functions.
class Definition {
public:
    Definition(Package *pkg, std::u32string documentation, std::u32string name, SourcePosition p)
        : package_(pkg), documentation_(std::move(documentation)), name_(std::move(name)), position_(std::move(p)) {}

    /// The name of the function.
    std::u32string name() const { return name_; }

    /// The package in which the defintion was made.
    Package* package() const { return package_; }

    /// Returns the position at which this function was defined.
    const SourcePosition& position() const { return position_; }

    const std::u32string& documentation() const { return documentation_; }

private:
    Package *package_;
    std::u32string documentation_;
    std::u32string name_;
    SourcePosition position_;
};

}

#endif /* Definition_hpp */
