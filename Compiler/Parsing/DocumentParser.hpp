//
//  DocumentParser.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 24/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef PackageParser_hpp
#define PackageParser_hpp

#include "AbstractParser.hpp"
#include "AttributesParser.hpp"

namespace EmojicodeCompiler {

using PackageAttributeParser = AttributeParser<Attribute::Export, Attribute::Final, Attribute::Foreign>;

/// DocumentParser instances parse the direct output from the lexer for one source code document (one source file).
/// parse() therefore expects $document-statement$s.
class DocumentParser : AbstractParser {
public:
    DocumentParser(Package *pkg, TokenStream stream, bool interface)
            : AbstractParser(pkg, stream_), interface_(interface), stream_(std::move(stream)) {}
    void parse();
private:
    bool interface_;
    TokenStream stream_;

    /// Parses a $type-identifier$ and ensures that a type with this name can be declared in the current package.
    /// This method is used with type declarations.
    TypeIdentifier parseAndValidateNewTypeName();
    
    /// Called if a $class$ has been detected. The first token has already been parsed.
    void parseClass(const std::u32string &documentation, const Token &theToken, bool exported, bool final,
                    bool foreign);
    /// Called if a $enum$ has been detected. The first token has already been parsed.
    void parseEnum(const std::u32string &documentation, const Token &theToken, bool exported);
    /// Called if a $protocol$ has been detected. The first token has already been parsed.
    void parseProtocol(const std::u32string &documentation, const Token &theToken, bool exported);
    /// Called if a $value-type$ has been detected. The first token has already been parsed.
    void parseValueType(const std::u32string &documentation, const Token &theToken, bool exported);
    /// Called if a $package-import$ has been detected. The first token has already been parsed.
    void parsePackageImport(const SourcePosition &p);
    /// Called if an $extension$ has been detected. The first token has already been parsed.
    void parseExtension(const Documentation &documentation, const SourcePosition &p);
    /// Called if an $include$ has been detected. The first token has already been parsed.
    void parseInclude(const SourcePosition &p);
    /// Called if a $version$ has been detected. The first token has already been parsed.
    void parseVersion(const Documentation &documentation, const SourcePosition &p);
    /// Called if a $start-flag$ has been detected. The first token has already been parsed.
    void parseStartFlag(const Documentation &documentation, const SourcePosition &p);
};

}  // namespace EmojicodeCompiler

#endif /* PackageParser_hpp */
