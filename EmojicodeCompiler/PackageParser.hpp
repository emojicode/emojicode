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
#include "AbstractParser.hpp"
#include "CompilerErrorException.hpp"
#include "utf8.h"

#include <set>

class PackageParser : AbstractParser {
public:
    PackageParser(Package *pkg, TokenStream stream) : AbstractParser(pkg, stream) {};
    void parse();
private:
    /**
     * Determines if the user has choosen a reserved method/initializer name and issues an error if necessary.
     * @param place The place in code (like "method")
     */
    void reservedEmojis(const Token &token, const char *place) const;
    /** Parses a type name and validates that it is not already in use or an optional. */
    const Token& parseAndValidateNewTypeName(EmojicodeChar *name, EmojicodeChar *ns);
    /** Parses the definition list of generic arguments for a type. */
    void parseGenericArgumentList(TypeDefinitionFunctional *typeDef, TypeContext tc);
    
    /** Parses a class definition, starting from the first token after 🐇. */
    void parseClass(const EmojicodeString &string, const Token &theToken, bool exported, bool final);
    /** Parses a enum defintion, starting from the first token after 🦃. */
    void parseEnum(const EmojicodeString &string, const Token &theToken, bool exported);
    /** Parses a protocol defintion, starting from the first token after🐊. */
    void parseProtocol(const EmojicodeString &string, const Token &theToken, bool exported);
    /** Parses a value type definition, starting from the first token after 🕊. */
    void parseValueType(const EmojicodeString &string, const Token &theToken, bool exported);
    
    /** Parses the body of a TypeDefinitionFunctional type. */
    void parseTypeDefinitionBody(Type typed, std::set<EmojicodeChar> *requiredInitializers, bool allowNative);
};

template<EmojicodeChar attributeName>
class Attribute {
public:
    Attribute& parse(TokenStream *tokenStream) {
        if (tokenStream->nextTokenIs(attributeName)) {
            position_ = tokenStream->consumeToken(TokenType::Identifier).position();
            set_ = true;
        }
        return *this;
    }
    bool set() const { return set_; }
    void disallow() const {
        if (set_) {
            ecCharToCharStack(attributeName, es)
            throw CompilerErrorException(position_, "Inapplicable attribute %s.", es);
        }
    }
private:
    bool set_ = false;
    SourcePosition position_ = SourcePosition(0, 0, "");
};

class Documentation {
public:
    Documentation& parse(TokenStream *tokenStream) {
        if (tokenStream->nextTokenIs(TokenType::DocumentationComment)) {
            auto &token = tokenStream->consumeToken(TokenType::DocumentationComment);
            position_ = token.position();
            documentation_ = token.value;
            found_ = true;
        }
        return *this;
    }
    const EmojicodeString& get() const { return documentation_; }
    void disallow() const {
        if (found_) {
            throw CompilerErrorException(position_, "Misplaced documentation token.");
        }
    }
private:
    EmojicodeString documentation_;
    bool found_ = false;
    SourcePosition position_ = SourcePosition(0, 0, "");
};

#endif /* PackageParser_hpp */
