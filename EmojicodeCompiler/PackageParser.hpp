//
//  PackageParser.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 24/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef PackageParser_hpp
#define PackageParser_hpp

#include "Package.hpp"
#include "Lexer.hpp"
#include "utf8.h"

#include <set>

class PackageParser {
public:
    PackageParser(Package *pkg) : package_(pkg) {};
    void parse(const char *mainFilePath);
private:
    /**
     * Determines if the user has choosen a reserved method/initializer name and issues an error if necessary.
     * @param place The place in code (like "method")
     */
    void reservedEmojis(const Token *token, const char *place) const;
    /** Parses a type name and validates that it is not already in use or an optional. */
    const Token* parseAndValidateTypeName(EmojicodeChar *name, EmojicodeChar *ns);
    /** Parses the definition list of generic arguments for a type. */
    void parseGenericArgumentList(TypeDefinitionWithGenerics *typeDef, TypeContext tc);
    
    /** Parses a class definition, starting from the first token after 🐇. */
    void parseClass(const Token *documentationToken, const Token *theToken, bool exported);
    /** Parses the body of a class. */
    void parseClassBody(Class *eclass, std::set<EmojicodeChar> *requiredInitializers, bool allowNative);
    /** Parses a enum defintion, starting from the first token after 🦃. */
    void parseEnum(const Token *documentationToken, bool exported);
    /** Parses a protocol defintion, starting from the first token after🐊. */
    void parseProtocol(const Token *documentationToken, bool exported);
    
    Package *package_;
};

template<EmojicodeChar attributeName>
class Attribute {
public:
    Attribute& parse(const Token **token) {
        if ((*token)->value[0] == attributeName) {
            *token = token_ = consumeToken(IDENTIFIER);
            set_ = true;
        }
        return *this;
    }
    bool set() const { return set_; }
    void disallow() const {
        if (set_) {
            ecCharToCharStack(attributeName, es)
            compilerError(token_, "Inapplicable attribute %s.", es);
        }
    }
private:
    bool set_ = false;
    const Token *token_;
};

#endif /* PackageParser_hpp */
