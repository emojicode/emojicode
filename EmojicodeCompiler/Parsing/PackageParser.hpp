//
//  PackageParser.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 24/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef PackageParser_hpp
#define PackageParser_hpp

#include "../CompilerError.hpp"
#include "../Lex/Lexer.hpp"
#include "AbstractParser.hpp"
#include "Package.hpp"

#include <set>

namespace EmojicodeCompiler {

class PackageParser : AbstractParser {
public:
    PackageParser(Package *pkg, TokenStream stream) : AbstractParser(pkg, stream) {}
    void parse();
private:
    /**
     * Determines if the user has choosen a reserved method/initializer name and issues an error if necessary.
     * @param place The place in code (like "method")
     */
    void reservedEmojis(const Token &token, const char *place) const;
    /** Parses a type name and validates that it is not already in use or an optional. */
    TypeIdentifier parseAndValidateNewTypeName();
    /** Parses the definition list of generic arguments for a type. */
    void parseGenericArgumentList(TypeDefinition *typeDef, const TypeContext& tc);
    
    /** Parses a class definition, starting from the first token after 🐇. */
    void parseClass(const std::u32string &documentation, const Token &theToken, bool exported, bool final);
    /** Parses a enum defintion, starting from the first token after 🦃. */
    void parseEnum(const std::u32string &documentation, const Token &theToken, bool exported);
    /** Parses a protocol defintion, starting from the first token after🐊. */
    void parseProtocol(const std::u32string &documentation, const Token &theToken, bool exported);
    /** Parses a value type definition, starting from the first token after 🕊. */
    void parseValueType(const std::u32string &documentation, const Token &theToken, bool exported);

    void parseFunction(Function *function);
    
    /** Parses the body of a TypeDefinitionFunctional type. */
    void parseTypeDefinitionBody(const Type &typed, std::set<std::u32string> *requiredInitializers);
};

template<char32_t attributeName>
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
            throw CompilerError(position_, "Inapplicable attribute %s.", utf8(std::u32string(1, attributeName)).c_str());
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
            documentation_ = token.value();
            found_ = true;
        }
        return *this;
    }
    const std::u32string& get() const { return documentation_; }
    void disallow() const {
        if (found_) {
            throw CompilerError(position_, "Misplaced documentation token.");
        }
    }
private:
    std::u32string documentation_;
    bool found_ = false;
    SourcePosition position_ = SourcePosition(0, 0, "");
};

}  // namespace EmojicodeCompiler

#endif /* PackageParser_hpp */
