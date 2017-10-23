//
//  EmojiTokenization.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 23/12/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef EmojiTokenization_hpp
#define EmojiTokenization_hpp

#include <string>

namespace EmojicodeCompiler {

/// Whether the character is an emoji or not. This method acts in conformance with Unicode Emoji v4.0, except for that
/// this method does not return true for #, *, and digits from 0 to 9.
/// http://unicode.org/emoji/charts/emoji-list.html and http://www.unicode.org/Public/emoji/4.0//emoji-data.txt
bool isEmoji(char32_t ch);

/// Whether the character is an emoji modifier base as defined in Unicode® Technical Report #51.
/// http://www.unicode.org/reports/tr51/#Emoji_Implementation_Notes
bool isEmojiModifierBase(char32_t ch);

/// Whether the character is an emoji modifier as defined in Unicode® Technical Report #51.
bool isEmojiModifier(char32_t ch);

bool isRegionalIndicator(char32_t ch);

bool isValidEmoji(std::u32string string);

}  // namespace EmojicodeCompiler

#endif /* EmojiTokenization_hpp */

