//
//  EmojiTokenization.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 23/12/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "EmojiTokenization.hpp"

namespace EmojicodeCompiler {

bool isEmoji(EmojicodeChar ch) {
    return (/* ch == 0x0023 || ch == 0x002A || (0x0030 <= ch && ch <= 0x0039) || */
            ch == 0x00A9 || ch == 0x00AE || ch == 0x203C ||
            ch == 0x2049 || ch == 0x2122 || ch == 0x2139 ||
            (0x2194 <= ch && ch <= 0x2199) || (0x21A9 <= ch && ch <= 0x21AA) || (0x231A <= ch && ch <= 0x231B) ||
            ch == 0x2328 || ch == 0x23CF || (0x23E9 <= ch && ch <= 0x23F3) ||
            (0x23F8 <= ch && ch <= 0x23FA) || ch == 0x24C2 || (0x25AA <= ch && ch <= 0x25AB) ||
            ch == 0x25B6 || ch == 0x25C0 || (0x25FB <= ch && ch <= 0x25FE) ||
            (0x2600 <= ch && ch <= 0x2604) || ch == 0x260E || ch == 0x2611 ||
            (0x2614 <= ch && ch <= 0x2615) || ch == 0x2618 || ch == 0x261D ||
            ch == 0x2620 || (0x2622 <= ch && ch <= 0x2623) || ch == 0x2626 ||
            ch == 0x262A || (0x262E <= ch && ch <= 0x262F) || (0x2638 <= ch && ch <= 0x263A) ||
            ch == 0x2640 || ch == 0x2642 || (0x2648 <= ch && ch <= 0x2653) ||
            ch == 0x2660 || ch == 0x2663 || (0x2665 <= ch && ch <= 0x2666) ||
            ch == 0x2668 || ch == 0x267B || ch == 0x267F ||
            (0x2692 <= ch && ch <= 0x2697) || ch == 0x2699 || (0x269B <= ch && ch <= 0x269C) ||
            (0x26A0 <= ch && ch <= 0x26A1) || (0x26AA <= ch && ch <= 0x26AB) || (0x26B0 <= ch && ch <= 0x26B1) ||
            (0x26BD <= ch && ch <= 0x26BE) || (0x26C4 <= ch && ch <= 0x26C5) || ch == 0x26C8 ||
            ch == 0x26CE || ch == 0x26CF || ch == 0x26D1 ||
            (0x26D3 <= ch && ch <= 0x26D4) || (0x26E9 <= ch && ch <= 0x26EA) || (0x26F0 <= ch && ch <= 0x26F5) ||
            (0x26F7 <= ch && ch <= 0x26FA) || ch == 0x26FD || ch == 0x2702 ||
            ch == 0x2705 || (0x2708 <= ch && ch <= 0x2709) || (0x270A <= ch && ch <= 0x270B) ||
            (0x270C <= ch && ch <= 0x270D) || ch == 0x270F || ch == 0x2712 ||
            ch == 0x2714 || ch == 0x2716 || ch == 0x271D ||
            ch == 0x2721 || ch == 0x2728 || (0x2733 <= ch && ch <= 0x2734) ||
            ch == 0x2744 || ch == 0x2747 || ch == 0x274C ||
            ch == 0x274E || (0x2753 <= ch && ch <= 0x2755) || ch == 0x2757 ||
            (0x2763 <= ch && ch <= 0x2764) || (0x2795 <= ch && ch <= 0x2797) || ch == 0x27A1 ||
            ch == 0x27B0 || ch == 0x27BF || (0x2934 <= ch && ch <= 0x2935) ||
            (0x2B05 <= ch && ch <= 0x2B07) || (0x2B1B <= ch && ch <= 0x2B1C) || ch == 0x2B50 ||
            ch == 0x2B55 || ch == 0x3030 || ch == 0x303D ||
            ch == 0x3297 || ch == 0x3299 || ch == 0x1F004 ||
            ch == 0x1F0CF || (0x1F170 <= ch && ch <= 0x1F171) || ch == 0x1F17E ||
            ch == 0x1F17F || ch == 0x1F18E || (0x1F191 <= ch && ch <= 0x1F19A) ||
            (0x1F1E6 <= ch && ch <= 0x1F1FF) || (0x1F201 <= ch && ch <= 0x1F202) || ch == 0x1F21A ||
            ch == 0x1F22F || (0x1F232 <= ch && ch <= 0x1F23A) || (0x1F250 <= ch && ch <= 0x1F251) ||
            (0x1F300 <= ch && ch <= 0x1F320) || ch == 0x1F321 || (0x1F324 <= ch && ch <= 0x1F32C) ||
            (0x1F32D <= ch && ch <= 0x1F32F) || (0x1F330 <= ch && ch <= 0x1F335) || ch == 0x1F336 ||
            (0x1F337 <= ch && ch <= 0x1F37C) || ch == 0x1F37D || (0x1F37E <= ch && ch <= 0x1F37F) ||
            (0x1F380 <= ch && ch <= 0x1F393) || (0x1F396 <= ch && ch <= 0x1F397) || (0x1F399 <= ch && ch <= 0x1F39B) ||
            (0x1F39E <= ch && ch <= 0x1F39F) || (0x1F3A0 <= ch && ch <= 0x1F3C4) || ch == 0x1F3C5 ||
            (0x1F3C6 <= ch && ch <= 0x1F3CA) || (0x1F3CB <= ch && ch <= 0x1F3CE) || (0x1F3CF <= ch && ch <= 0x1F3D3) ||
            (0x1F3D4 <= ch && ch <= 0x1F3DF) || (0x1F3E0 <= ch && ch <= 0x1F3F0) || (0x1F3F3 <= ch && ch <= 0x1F3F5) ||
            ch == 0x1F3F7 || (0x1F3F8 <= ch && ch <= 0x1F3FF) || (0x1F400 <= ch && ch <= 0x1F43E) ||
            ch == 0x1F43F || ch == 0x1F440 || ch == 0x1F441 ||
            (0x1F442 <= ch && ch <= 0x1F4F7) || ch == 0x1F4F8 || (0x1F4F9 <= ch && ch <= 0x1F4FC) ||
            ch == 0x1F4FD || ch == 0x1F4FF || (0x1F500 <= ch && ch <= 0x1F53D) ||
            (0x1F549 <= ch && ch <= 0x1F54A) || (0x1F54B <= ch && ch <= 0x1F54E) || (0x1F550 <= ch && ch <= 0x1F567) ||
            (0x1F56F <= ch && ch <= 0x1F570) || (0x1F573 <= ch && ch <= 0x1F579) || ch == 0x1F57A ||
            ch == 0x1F587 || (0x1F58A <= ch && ch <= 0x1F58D) || ch == 0x1F590 ||
            (0x1F595 <= ch && ch <= 0x1F596) || ch == 0x1F5A4 || ch == 0x1F5A5 ||
            ch == 0x1F5A8 || (0x1F5B1 <= ch && ch <= 0x1F5B2) || ch == 0x1F5BC ||
            (0x1F5C2 <= ch && ch <= 0x1F5C4) || (0x1F5D1 <= ch && ch <= 0x1F5D3) || (0x1F5DC <= ch && ch <= 0x1F5DE) ||
            ch == 0x1F5E1 || ch == 0x1F5E3 || ch == 0x1F5E8 ||
            ch == 0x1F5EF || ch == 0x1F5F3 || ch == 0x1F5FA ||
            (0x1F5FB <= ch && ch <= 0x1F5FF) || ch == 0x1F600 || (0x1F601 <= ch && ch <= 0x1F610) ||
            ch == 0x1F611 || (0x1F612 <= ch && ch <= 0x1F614) || ch == 0x1F615 ||
            ch == 0x1F616 || ch == 0x1F617 || ch == 0x1F618 ||
            ch == 0x1F619 || ch == 0x1F61A || ch == 0x1F61B ||
            (0x1F61C <= ch && ch <= 0x1F61E) || ch == 0x1F61F || (0x1F620 <= ch && ch <= 0x1F625) ||
            (0x1F626 <= ch && ch <= 0x1F627) || (0x1F628 <= ch && ch <= 0x1F62B) || ch == 0x1F62C ||
            ch == 0x1F62D || (0x1F62E <= ch && ch <= 0x1F62F) || (0x1F630 <= ch && ch <= 0x1F633) ||
            ch == 0x1F634 || (0x1F635 <= ch && ch <= 0x1F640) || (0x1F641 <= ch && ch <= 0x1F642) ||
            (0x1F643 <= ch && ch <= 0x1F644) || (0x1F645 <= ch && ch <= 0x1F64F) || (0x1F680 <= ch && ch <= 0x1F6C5) ||
            (0x1F6CB <= ch && ch <= 0x1F6CF) || ch == 0x1F6D0 || (0x1F6D1 <= ch && ch <= 0x1F6D2) ||
            (0x1F6E0 <= ch && ch <= 0x1F6E5) || ch == 0x1F6E9 || (0x1F6EB <= ch && ch <= 0x1F6EC) ||
            ch == 0x1F6F0 || ch == 0x1F6F3 || (0x1F6F4 <= ch && ch <= 0x1F6F6) ||
            (0x1F910 <= ch && ch <= 0x1F918) || (0x1F919 <= ch && ch <= 0x1F91E) || (0x1F920 <= ch && ch <= 0x1F927) ||
            ch == 0x1F930 || (0x1F933 <= ch && ch <= 0x1F93A) || (0x1F93C <= ch && ch <= 0x1F93E) ||
            (0x1F940 <= ch && ch <= 0x1F945) || (0x1F947 <= ch && ch <= 0x1F94B) || (0x1F950 <= ch && ch <= 0x1F95E) ||
            (0x1F980 <= ch && ch <= 0x1F984) || (0x1F985 <= ch && ch <= 0x1F991) || ch == 0x1F9C0);
}

bool isEmojiModifierBase(EmojicodeChar ch) {
    return (ch == 0x261D || ch == 0x26F9 || (0x270A <= ch && ch <= 0x270B) ||
            (0x270C <= ch && ch <= 0x270D) || ch == 0x1F385 || (0x1F3C2 <= ch && ch <= 0x1F3C4) ||
            ch == 0x1F3C7 || ch == 0x1F3CA || (0x1F3CB <= ch && ch <= 0x1F3CC) ||
            (0x1F442 <= ch && ch <= 0x1F443) || (0x1F446 <= ch && ch <= 0x1F450) || (0x1F466 <= ch && ch <= 0x1F469) ||
            ch == 0x1F46E || (0x1F470 <= ch && ch <= 0x1F478) || ch == 0x1F47C ||
            (0x1F481 <= ch && ch <= 0x1F483) || (0x1F485 <= ch && ch <= 0x1F487) || ch == 0x1F4AA ||
            (0x1F574 <= ch && ch <= 0x1F575) || ch == 0x1F57A || ch == 0x1F590 ||
            (0x1F595 <= ch && ch <= 0x1F596) || (0x1F645 <= ch && ch <= 0x1F647) || (0x1F64B <= ch && ch <= 0x1F64F) ||
            ch == 0x1F6A3 || (0x1F6B4 <= ch && ch <= 0x1F6B6) || ch == 0x1F6C0 ||
            ch == 0x1F6CC || ch == 0x1F918 || (0x1F919 <= ch && ch <= 0x1F91C) ||
            ch == 0x1F91E || ch == 0x1F926 || ch == 0x1F930 ||
            (0x1F933 <= ch && ch <= 0x1F939) || (0x1F93D <= ch && ch <= 0x1F93E));
}

bool isEmojiModifier(EmojicodeChar ch) {
    return 0x1F3FB <= ch && ch <= 0x1F3FF;
}

bool isRegionalIndicator(EmojicodeChar ch) {
    return 0x1F1E6 <= ch && ch <= 0x1F1FF;
}

bool isValidEmoji(EmojicodeString string) {
    if (string.size() == 1) {
        return !isRegionalIndicator(string.front());
    }
    return string.back() != 0x200D;
}

}  // namespace EmojicodeCompiler
