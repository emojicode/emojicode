//
//  TypeDefinition.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef TypeDefinition_hpp
#define TypeDefinition_hpp

#include <map>
#include "EmojicodeCompiler.hpp"
#include "CompilerErrorException.hpp"
#include "Token.hpp"
#include "utf8.h"

class TypeDefinition {
public:
    /** Returns a documentation token documenting this type definition or @c nullptr. */
    const EmojicodeString& documentation() const { return documentation_; }
    /** Returns the name of the type definition. */
    EmojicodeChar name() const { return name_; }
    /** Returns the package in which this type was defined. */
    Package* package() const { return package_; }
    /** The position at which this type was initially defined. */
    const SourcePosition& position() const { return position_; }
    /** Computes the type definition’s size and other values that must be
        computed before functional code can be compiled. */
    virtual void finalize() {}
protected:
    TypeDefinition(EmojicodeChar name, Package *p, SourcePosition position, const EmojicodeString &doc)
        : name_(name),
          package_(p),
          documentation_(doc),
          position_(position) {}
    template <typename T>
    void duplicateDeclarationCheck(T p, std::map<EmojicodeChar, T> dict, SourcePosition position) {
        if (dict.count(p->name())) {
            ecCharToCharStack(p->name(), nameString);
            throw CompilerErrorException(position, "%s is declared twice.", nameString);
        }
    }
private:
    EmojicodeChar name_;
    Package *package_;
    EmojicodeString documentation_;
    SourcePosition position_;
};

#endif /* TypeDefinition_hpp */
