//
//  AttributesParser.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 24/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef AttributesParser_hpp
#define AttributesParser_hpp

#include "Compiler.hpp"
#include "Emojis.h"
#include "Lex/TokenStream.hpp"
#include "CompilerError.hpp"
#include <array>
#include <map>
#include <string>

namespace EmojicodeCompiler {

struct SourcePosition;
class TokenStream;

enum class Attribute : char32_t {
    Deprecated = E_WARNING_SIGN, Final = E_LOCK_WITH_INK_PEN, Override = E_BLACK_NIB, StaticOnType = E_RABBIT,
    Required = E_KEY, Export = E_EARTH_GLOBE_EUROPE_AFRICA, Foreign = E_RADIO, Unsafe = E_BIOHAZARD,
    Mutating = E_CRAYON, Escaping = E_TAKEOUT_BOX, Inline = E_BAGEL,
};

template <Attribute ...Attributes>
class AttributeParser {
public:
    AttributeParser& allow(Attribute attr) { found_.find(attr)->second.allowed = true; return *this; }
    bool has(Attribute attr) const { return found_.find(attr)->second.found; }

    void check(const SourcePosition &p, Compiler *compiler) const {
        for (auto &pair : found_) {
            if (!pair.second.allowed && pair.second.found) {
                auto name = utf8(std::u32string(1, static_cast<char32_t>(pair.first)));
                compiler->error(CompilerError(p, "Attribute ", name, " not applicable."));
            }
        }
    }

    AttributeParser& parse(TokenStream *stream) {
        for (auto attr : order_) {
            auto found = test(stream, attr);
            found_.emplace(attr, FoundAttribute(found));
        }
        return *this;
    }
private:
    struct FoundAttribute {
        explicit FoundAttribute(bool found) : found(found) {}
        bool found;
        bool allowed = false;
    };

    bool test(TokenStream *stream, Attribute attr) const {
        switch (attr) {
            case Attribute::Mutating:
                return stream->consumeTokenIf(TokenType::Mutable);
            case Attribute ::Unsafe:
                return stream->consumeTokenIf(TokenType::Unsafe);
            case Attribute::StaticOnType:
                return stream->consumeTokenIf(TokenType::Class);
            case Attribute::Escaping:
                return stream->consumeTokenIf(E_TAKEOUT_BOX, TokenType::Decorator);
            default:
                return stream->consumeTokenIf(static_cast<char32_t>(attr));
        }
    }

    constexpr static const std::array<Attribute, sizeof...(Attributes)> order_ = { Attributes... };
    std::map<Attribute, FoundAttribute> found_;
};

template <Attribute ...Attributes>
constexpr const std::array<Attribute, sizeof...(Attributes)> AttributeParser<Attributes...>::order_;

}  // namespace EmojicodeCompiler

#endif /* AttributesParser_hpp */
