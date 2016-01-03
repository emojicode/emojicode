//
//  KeycodeToChar.c
//  DefaultPackages
//
//  Created by Theo Weidmann on 01/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "SDLPackage.h"

EmojicodeChar keyCodeToCharSwitch(SDL_Keycode code){
    switch (code) {
        case SDLK_BACKSPACE: // '"Backspace"'
            return 0x08; // 8
        case SDLK_TAB: // '"Tab" (the Tab key)'
            return 0x09; // 9
        case SDLK_RETURN: // '"Return" (the Enter key (main keyboard))'
            return 0x0A; // before: 0x0D (13)
        case SDLK_ESCAPE: // '"Escape" (the Esc key)'
            return 0x238B; // before: 0x1B (27)
        case SDLK_SPACE: // '"Space" (the Space Bar key(s))'
            return 0x20; // 32
        case SDLK_EXCLAIM: // '"!"'
            return 0x21; // 33
        case SDLK_QUOTEDBL: // '"""'
            return 0x22; // 34
        case SDLK_HASH: // '"#"'
            return 0x23; // 35
        case SDLK_DOLLAR: // '"$"'
            return 0x24; // 36
        case SDLK_PERCENT: // '"%"'
            return 0x25; // 37
        case SDLK_AMPERSAND: // '"&"'
            return 0x26; // 38
        case SDLK_QUOTE: // '"\'"'
            return 0x27; // 39
        case SDLK_LEFTPAREN: // '"("'
            return 0x28; // 40
        case SDLK_RIGHTPAREN: // '")"'
            return 0x29; // 41
        case SDLK_ASTERISK: // '"*"'
            return 0x2A; // 42
        case SDLK_PLUS: // '"+"'
            return 0x2B; // 43
        case SDLK_COMMA: // '","'
            return 0x2C; // 44
        case SDLK_MINUS: // '"-"'
            return 0x2D; // 45
        case SDLK_PERIOD: // '"."'
            return 0x2E; // 46
        case SDLK_SLASH: // '"/"'
            return 0x2F; // 47
        case SDLK_0: // '"0"'
            return 0x30; // 48
        case SDLK_1: // '"1"'
            return 0x31; // 49
        case SDLK_2: // '"2"'
            return 0x32; // 50
        case SDLK_3: // '"3"'
            return 0x33; // 51
        case SDLK_4: // '"4"'
            return 0x34; // 52
        case SDLK_5: // '"5"'
            return 0x35; // 53
        case SDLK_6: // '"6"'
            return 0x36; // 54
        case SDLK_7: // '"7"'
            return 0x37; // 55
        case SDLK_8: // '"8"'
            return 0x38; // 56
        case SDLK_9: // '"9"'
            return 0x39; // 57
        case SDLK_COLON: // '":"'
            return 0x3A; // 58
        case SDLK_SEMICOLON: // '";"'
            return 0x3B; // 59
        case SDLK_LESS: // '"<"'
            return 0x3C; // 60
        case SDLK_EQUALS: // '"="'
            return 0x3D; // 61
        case SDLK_GREATER: // '">"'
            return 0x3E; // 62
        case SDLK_QUESTION: // '"?"'
            return 0x3F; // 63
        case SDLK_AT: // '"@"'
            return 0x40; // 64
        case SDLK_LEFTBRACKET: // '"["'
            return 0x5B; // 91
        case SDLK_BACKSLASH: // '"\\" (Located at the lower left of the return key on ISO keyboards and at the right end of the QWERTY row on ANSI keyboards. Produces REVERSE SOLIDUS (backslash) and VERTICAL LINE in a US layout, REVERSE SOLIDUS and VERTICAL LINE in a UK Mac layout, NUMBER SIGN and TILDE in a UK Windows layout, DOLLAR SIGN and POUND SIGN in a Swiss German layout, NUMBER SIGN and APOSTROPHE in a German layout, GRAVE ACCENT and POUND SIGN in a French Mac layout, and ASTERISK and MICRO SIGN in a French Windows layout.)'
            return 0x5C; // 92
        case SDLK_RIGHTBRACKET: // '"]"'
            return 0x5D; // 93
        case SDLK_CARET: // '"^"'
            return 0x5E; // 94
        case SDLK_UNDERSCORE: // '"_"'
            return 0x5F; // 95
        case SDLK_BACKQUOTE: // '"`" (Located in the top left corner (on both ANSI and ISO keyboards). Produces GRAVE ACCENT and TILDE in a US Windows layout and in US and UK Mac layouts on ANSI keyboards, GRAVE ACCENT and NOT SIGN in a UK Windows layout, SECTION SIGN and PLUS-MINUS SIGN in US and UK Mac layouts on ISO keyboards, SECTION SIGN and DEGREE SIGN in a Swiss German layout (Mac: only on ISO keyboards), CIRCUMFLEX ACCENT and DEGREE SIGN in a German layout (Mac: only on ISO keyboards), SUPERSCRIPT TWO and TILDE in a French Windows layout, COMMERCIAL AT and NUMBER SIGN in a French Mac layout on ISO keyboards, and LESS-THAN SIGN and GREATER-THAN SIGN in a Swiss German, German, or French Mac layout on ANSI keyboards.)'
            return 0x60; // 96
        case SDLK_a: // '"A"'
            return 0x61; // 97
        case SDLK_b: // '"B"'
            return 0x62; // 98
        case SDLK_c: // '"C"'
            return 0x63; // 99
        case SDLK_d: // '"D"'
            return 0x64; // 100
        case SDLK_e: // '"E"'
            return 0x65; // 101
        case SDLK_f: // '"F"'
            return 0x66; // 102
        case SDLK_g: // '"G"'
            return 0x67; // 103
        case SDLK_h: // '"H"'
            return 0x68; // 104
        case SDLK_i: // '"I"'
            return 0x69; // 105
        case SDLK_j: // '"J"'
            return 0x6A; // 106
        case SDLK_k: // '"K"'
            return 0x6B; // 107
        case SDLK_l: // '"L"'
            return 0x6C; // 108
        case SDLK_m: // '"M"'
            return 0x6D; // 109
        case SDLK_n: // '"N"'
            return 0x6E; // 110
        case SDLK_o: // '"O"'
            return 0x6F; // 111
        case SDLK_p: // '"P"'
            return 0x70; // 112
        case SDLK_q: // '"Q"'
            return 0x71; // 113
        case SDLK_r: // '"R"'
            return 0x72; // 114
        case SDLK_s: // '"S"'
            return 0x73; // 115
        case SDLK_t: // '"T"'
            return 0x74; // 116
        case SDLK_u: // '"U"'
            return 0x75; // 117
        case SDLK_v: // '"V"'
            return 0x76; // 118
        case SDLK_w: // '"W"'
            return 0x77; // 119
        case SDLK_x: // '"X"'
            return 0x78; // 120
        case SDLK_y: // '"Y"'
            return 0x79; // 121
        case SDLK_z: // '"Z"'
            return 0x7A; // 122
        case SDLK_DELETE: // '"Delete"'
            return 0x2326; // before: 0x7F (127)
        case SDLK_CAPSLOCK: // '"CapsLock"'
            return 0x21EA; // before: 0x40000039 (1073741881)
        case SDLK_F1: // '"F1"'
            return 0x4000003A; // 1073741882
        case SDLK_F2: // '"F2"'
            return 0x4000003B; // 1073741883
        case SDLK_F3: // '"F3"'
            return 0x4000003C; // 1073741884
        case SDLK_F4: // '"F4"'
            return 0x4000003D; // 1073741885
        case SDLK_F5: // '"F5"'
            return 0x4000003E; // 1073741886
        case SDLK_F6: // '"F6"'
            return 0x4000003F; // 1073741887
        case SDLK_F7: // '"F7"'
            return 0x40000040; // 1073741888
        case SDLK_F8: // '"F8"'
            return 0x40000041; // 1073741889
        case SDLK_F9: // '"F9"'
            return 0x40000042; // 1073741890
        case SDLK_F10: // '"F10"'
            return 0x40000043; // 1073741891
        case SDLK_F11: // '"F11"'
            return 0x40000044; // 1073741892
        case SDLK_F12: // '"F12"'
            return 0x40000045; // 1073741893
        case SDLK_PRINTSCREEN: // '"PrintScreen"'
            return 0x2399; // before: 0x40000046 (1073741894)
        case SDLK_SCROLLLOCK: // '"ScrollLock"'
            return 0x40000047; // 1073741895
        case SDLK_PAUSE: // '"Pause" (the Pause / Break key)'
            return 0x2389; // before: 0x40000048 (1073741896)
        case SDLK_INSERT: // '"Insert" (insert on PC, help on some Mac keyboards (but does send code 73, not 117))'
            return 0x2380; // before: 0x40000049 (1073741897)
        case SDLK_HOME: // '"Home"'
            return 0x2196; // before: 0x4000004A (1073741898)
        case SDLK_PAGEUP: // '"PageUp"'
            return 0x21DE; // before: 0x4000004B (1073741899)
        case SDLK_END: // '"End"'
            return 0x2198; // before: 0x4000004D (1073741901)
        case SDLK_PAGEDOWN: // '"PageDown"'
            return 0x21DF; // before: 0x4000004E (1073741902)
        case SDLK_RIGHT: // '"Right" (the Right arrow key (navigation keypad))'
            return 0x2192; // before: 0x4000004F (1073741903)
        case SDLK_LEFT: // '"Left" (the Left arrow key (navigation keypad))'
            return 0x2190; // before: 0x40000050 (1073741904)
        case SDLK_DOWN: // '"Down" (the Down arrow key (navigation keypad))'
            return 0x2193; // before: 0x40000051 (1073741905)
        case SDLK_UP: // '"Up" (the Up arrow key (navigation keypad))'
            return 0x2191; // before: 0x40000052 (1073741906)
        case SDLK_NUMLOCKCLEAR: // '"Numlock" (the Num Lock key (PC) / the Clear key (Mac))'
            return 0x239A; // before: 0x40000053 (1073741907)
        case SDLK_KP_DIVIDE: // '"Keypad /" (the / key (numeric keypad))'
            return 0x40000054; // 1073741908
        case SDLK_KP_MULTIPLY: // '"Keypad *" (the * key (numeric keypad))'
            return 0x40000055; // 1073741909
        case SDLK_KP_MINUS: // '"Keypad -" (the - key (numeric keypad))'
            return 0x40000056; // 1073741910
        case SDLK_KP_PLUS: // '"Keypad +" (the + key (numeric keypad))'
            return 0x40000057; // 1073741911
        case SDLK_KP_ENTER: // '"Keypad Enter" (the Enter key (numeric keypad))'
            return 0x40000058; // 1073741912
        case SDLK_KP_1: // '"Keypad 1" (the 1 key (numeric keypad))'
            return 0x40000059; // 1073741913
        case SDLK_KP_2: // '"Keypad 2" (the 2 key (numeric keypad))'
            return 0x4000005A; // 1073741914
        case SDLK_KP_3: // '"Keypad 3" (the 3 key (numeric keypad))'
            return 0x4000005B; // 1073741915
        case SDLK_KP_4: // '"Keypad 4" (the 4 key (numeric keypad))'
            return 0x4000005C; // 1073741916
        case SDLK_KP_5: // '"Keypad 5" (the 5 key (numeric keypad))'
            return 0x4000005D; // 1073741917
        case SDLK_KP_6: // '"Keypad 6" (the 6 key (numeric keypad))'
            return 0x4000005E; // 1073741918
        case SDLK_KP_7: // '"Keypad 7" (the 7 key (numeric keypad))'
            return 0x4000005F; // 1073741919
        case SDLK_KP_8: // '"Keypad 8" (the 8 key (numeric keypad))'
            return 0x40000060; // 1073741920
        case SDLK_KP_9: // '"Keypad 9" (the 9 key (numeric keypad))'
            return 0x40000061; // 1073741921
        case SDLK_KP_0: // '"Keypad 0" (the 0 key (numeric keypad))'
            return 0x40000062; // 1073741922
        case SDLK_KP_PERIOD: // '"Keypad ." (the . key (numeric keypad))'
            return 0x40000063; // 1073741923
        case SDLK_APPLICATION: // '"Application" (the Application / Compose / Context Menu (Windows) key )'
            return 0x40000065; // 1073741925
        case SDLK_POWER: // '"Power" (The USB document says this is a status flag, not a physical key - but some Mac keyboards do have a power key.)'
            return 0x40000066; // 1073741926
        case SDLK_KP_EQUALS: // '"Keypad =" (the = key (numeric keypad))'
            return 0x40000067; // 1073741927
        case SDLK_F13: // '"F13"'
            return 0x40000068; // 1073741928
        case SDLK_F14: // '"F14"'
            return 0x40000069; // 1073741929
        case SDLK_F15: // '"F15"'
            return 0x4000006A; // 1073741930
        case SDLK_F16: // '"F16"'
            return 0x4000006B; // 1073741931
        case SDLK_F17: // '"F17"'
            return 0x4000006C; // 1073741932
        case SDLK_F18: // '"F18"'
            return 0x4000006D; // 1073741933
        case SDLK_F19: // '"F19"'
            return 0x4000006E; // 1073741934
        case SDLK_F20: // '"F20"'
            return 0x4000006F; // 1073741935
        case SDLK_F21: // '"F21"'
            return 0x40000070; // 1073741936
        case SDLK_F22: // '"F22"'
            return 0x40000071; // 1073741937
        case SDLK_F23: // '"F23"'
            return 0x40000072; // 1073741938
        case SDLK_F24: // '"F24"'
            return 0x40000073; // 1073741939
        case SDLK_EXECUTE: // '"Execute"'
            return 0x40000074; // 1073741940
        case SDLK_HELP: // '"Help"'
            return 0x2753; // before: 0x40000075 (1073741941)
        case SDLK_MENU: // '"Menu"'
            return 0x25A4; // before: 0x40000076 (1073741942)
        case SDLK_SELECT: // '"Select"'
            return 0x40000077; // 1073741943
        case SDLK_STOP: // '"Stop"'
            return 0x25FC; // before: 0x40000078 (1073741944)
        case SDLK_AGAIN: // '"Again" (the Again key (Redo))'
            return 0x21B7; // before: 0x40000079 (1073741945)
        case SDLK_UNDO: // '"Undo"'
            return 0x21B6; // before: 0x4000007A (1073741946)
        case SDLK_CUT: // '"Cut"'
            return 0x2704; // before: 0x4000007B (1073741947)
        case SDLK_COPY: // '"Copy"'
            return 0xA9; // before: 0x4000007C (1073741948)
        case SDLK_PASTE: // '"Paste"'
            return 0x4000007D; // 1073741949
        case SDLK_FIND: // '"Find"'
            return 0x1F50D; // before: 0x4000007E (1073741950)
        case SDLK_MUTE: // '"Mute"'
            return 0x1F507; // before: 0x4000007F (1073741951)
        case SDLK_VOLUMEUP: // '"VolumeUp"'
            return 0x1F50A; // before: 0x40000080 (1073741952)
        case SDLK_VOLUMEDOWN: // '"VolumeDown"'
            return 0x1F509; // before: 0x40000081 (1073741953)
        case SDLK_KP_COMMA: // '"Keypad ," (the Comma key (numeric keypad))'
            return 0x40000085; // 1073741957
        case SDLK_KP_EQUALSAS400: // '"Keypad = (AS400)" (the Equals AS400 key (numeric keypad))'
            return 0x40000086; // 1073741958
        case SDLK_ALTERASE: // '"AltErase" (Erase-Eaze)'
            return 0x40000099; // 1073741977
        case SDLK_SYSREQ: // '"SysReq" (the SysReq key)'
            return 0x4000009A; // 1073741978
        case SDLK_CANCEL: // '"Cancel"'
            return 0x4000009B; // 1073741979
        case SDLK_CLEAR: // '"Clear"'
            return 0x2327; // before: 0x4000009C (1073741980)
        case SDLK_PRIOR: // '"Prior"'
            return 0x4000009D; // 1073741981
        case SDLK_RETURN2: // '"Return"'
            return 0x4000009E; // 1073741982
        case SDLK_SEPARATOR: // '"Separator"'
            return 0x4000009F; // 1073741983
        case SDLK_OUT: // '"Out"'
            return 0x400000A0; // 1073741984
        case SDLK_OPER: // '"Oper"'
            return 0x400000A1; // 1073741985
        case SDLK_CLEARAGAIN: // '"Clear / Again"'
            return 0x400000A2; // 1073741986
        case SDLK_CRSEL: // '"CrSel"'
            return 0x400000A3; // 1073741987
        case SDLK_EXSEL: // '"ExSel"'
            return 0x400000A4; // 1073741988
        case SDLK_KP_00: // '"Keypad 00" (the 00 key (numeric keypad))'
            return 0x400000B0; // 1073742000
        case SDLK_KP_000: // '"Keypad 000" (the 000 key (numeric keypad))'
            return 0x400000B1; // 1073742001
        case SDLK_THOUSANDSSEPARATOR: // '"ThousandsSeparator" (the Thousands Separator key)'
            return 0x400000B2; // 1073742002
        case SDLK_DECIMALSEPARATOR: // '"DecimalSeparator" (the Decimal Separator key)'
            return 0x400000B3; // 1073742003
        case SDLK_CURRENCYUNIT: // '"CurrencyUnit" (the Currency Unit key)'
            return 0x400000B4; // 1073742004
        case SDLK_CURRENCYSUBUNIT: // '"CurrencySubUnit" (the Currency Subunit key)'
            return 0x400000B5; // 1073742005
        case SDLK_KP_LEFTPAREN: // '"Keypad (" (the Left Parenthesis key (numeric keypad))'
            return 0x400000B6; // 1073742006
        case SDLK_KP_RIGHTPAREN: // '"Keypad )" (the Right Parenthesis key (numeric keypad))'
            return 0x400000B7; // 1073742007
        case SDLK_KP_LEFTBRACE: // '"Keypad {" (the Left Brace key (numeric keypad))'
            return 0x400000B8; // 1073742008
        case SDLK_KP_RIGHTBRACE: // '"Keypad }" (the Right Brace key (numeric keypad))'
            return 0x400000B9; // 1073742009
        case SDLK_KP_TAB: // '"Keypad Tab" (the Tab key (numeric keypad))'
            return 0x400000BA; // 1073742010
        case SDLK_KP_BACKSPACE: // '"Keypad Backspace" (the Backspace key (numeric keypad))'
            return 0x400000BB; // 1073742011
        case SDLK_KP_A: // '"Keypad A" (the A key (numeric keypad))'
            return 0x400000BC; // 1073742012
        case SDLK_KP_B: // '"Keypad B" (the B key (numeric keypad))'
            return 0x400000BD; // 1073742013
        case SDLK_KP_C: // '"Keypad C" (the C key (numeric keypad))'
            return 0x400000BE; // 1073742014
        case SDLK_KP_D: // '"Keypad D" (the D key (numeric keypad))'
            return 0x400000BF; // 1073742015
        case SDLK_KP_E: // '"Keypad E" (the E key (numeric keypad))'
            return 0x400000C0; // 1073742016
        case SDLK_KP_F: // '"Keypad F" (the F key (numeric keypad))'
            return 0x400000C1; // 1073742017
        case SDLK_KP_XOR: // '"Keypad XOR" (the XOR key (numeric keypad))'
            return 0x400000C2; // 1073742018
        case SDLK_KP_POWER: // '"Keypad ^" (the Power key (numeric keypad))'
            return 0x400000C3; // 1073742019
        case SDLK_KP_PERCENT: // '"Keypad %" (the Percent key (numeric keypad))'
            return 0x400000C4; // 1073742020
        case SDLK_KP_LESS: // '"Keypad <" (the Less key (numeric keypad))'
            return 0x400000C5; // 1073742021
        case SDLK_KP_GREATER: // '"Keypad >" (the Greater key (numeric keypad))'
            return 0x400000C6; // 1073742022
        case SDLK_KP_AMPERSAND: // '"Keypad &" (the & key (numeric keypad))'
            return 0x400000C7; // 1073742023
        case SDLK_KP_DBLAMPERSAND: // '"Keypad &&" (the && key (numeric keypad))'
            return 0x400000C8; // 1073742024
        case SDLK_KP_VERTICALBAR: // '"Keypad |" (the | key (numeric keypad))'
            return 0x400000C9; // 1073742025
        case SDLK_KP_DBLVERTICALBAR: // '"Keypad ||" (the || key (numeric keypad))'
            return 0x400000CA; // 1073742026
        case SDLK_KP_COLON: // '"Keypad :" (the : key (numeric keypad))'
            return 0x400000CB; // 1073742027
        case SDLK_KP_HASH: // '"Keypad #" (the # key (numeric keypad))'
            return 0x400000CC; // 1073742028
        case SDLK_KP_SPACE: // '"Keypad Space" (the Space key (numeric keypad))'
            return 0x400000CD; // 1073742029
        case SDLK_KP_AT: // '"Keypad @" (the @ key (numeric keypad))'
            return 0x400000CE; // 1073742030
        case SDLK_KP_EXCLAM: // '"Keypad !" (the ! key (numeric keypad))'
            return 0x400000CF; // 1073742031
        case SDLK_KP_MEMSTORE: // '"Keypad MemStore" (the Mem Store key (numeric keypad))'
            return 0x400000D0; // 1073742032
        case SDLK_KP_MEMRECALL: // '"Keypad MemRecall" (the Mem Recall key (numeric keypad))'
            return 0x400000D1; // 1073742033
        case SDLK_KP_MEMCLEAR: // '"Keypad MemClear" (the Mem Clear key (numeric keypad))'
            return 0x400000D2; // 1073742034
        case SDLK_KP_MEMADD: // '"Keypad MemAdd" (the Mem Add key (numeric keypad))'
            return 0x400000D3; // 1073742035
        case SDLK_KP_MEMSUBTRACT: // '"Keypad MemSubtract" (the Mem Subtract key (numeric keypad))'
            return 0x400000D4; // 1073742036
        case SDLK_KP_MEMMULTIPLY: // '"Keypad MemMultiply" (the Mem Multiply key (numeric keypad))'
            return 0x400000D5; // 1073742037
        case SDLK_KP_MEMDIVIDE: // '"Keypad MemDivide" (the Mem Divide key (numeric keypad))'
            return 0x400000D6; // 1073742038
        case SDLK_KP_PLUSMINUS: // '"Keypad +/-" (the +/- key (numeric keypad))'
            return 0x400000D7; // 1073742039
        case SDLK_KP_CLEAR: // '"Keypad Clear" (the Clear key (numeric keypad))'
            return 0x400000D8; // 1073742040
        case SDLK_KP_CLEARENTRY: // '"Keypad ClearEntry" (the Clear Entry key (numeric keypad))'
            return 0x400000D9; // 1073742041
        case SDLK_KP_BINARY: // '"Keypad Binary" (the Binary key (numeric keypad))'
            return 0x400000DA; // 1073742042
        case SDLK_KP_OCTAL: // '"Keypad Octal" (the Octal key (numeric keypad))'
            return 0x400000DB; // 1073742043
        case SDLK_KP_DECIMAL: // '"Keypad Decimal" (the Decimal key (numeric keypad))'
            return 0x400000DC; // 1073742044
        case SDLK_KP_HEXADECIMAL: // '"Keypad Hexadecimal" (the Hexadecimal key (numeric keypad))'
            return 0x400000DD; // 1073742045
        case SDLK_LCTRL: // '"Left Ctrl"'
            return 0x2732; // before: 0x400000E0 (1073742048)
        case SDLK_LSHIFT: // '"Left Shift"'
            return 0x21E7; // before: 0x400000E1 (1073742049)
        case SDLK_LALT: // '"Left Alt" (alt, option)'
            return 0x2387; // before: 0x400000E2 (1073742050)
        case SDLK_LGUI: // '"Left GUI" (windows, command (apple), meta)'
            return 0x2318; // before: 0x400000E3 (1073742051)
        case SDLK_RCTRL: // '"Right Ctrl"'
            return 0x2732; // before: 0x400000E4 (1073742052)
        case SDLK_RSHIFT: // '"Right Shift"'
            return 0x21E7; // before: 0x400000E5 (1073742053)
        case SDLK_RALT: // '"Right Alt" (alt gr, option)'
            return 0x2387; // before: 0x400000E6 (1073742054)
        case SDLK_RGUI: // '"Right GUI" (windows, command (apple), meta)'
            return 0x2318; // before: 0x400000E7 (1073742055)
        case SDLK_MODE: // '"ModeSwitch" (I\'m not sure if this is really not covered by any of the above, but since there\'s a special KMOD_MODE for it I\'m adding it here)'
            return 0x40000101; // 1073742081
        case SDLK_AUDIONEXT: // '"AudioNext" (the Next Track media key)'
            return 0x23ED; // before: 0x40000102 (1073742082)
        case SDLK_AUDIOPREV: // '"AudioPrev" (the Previous Track media key)'
            return 0x23EE; // before: 0x40000103 (1073742083)
        case SDLK_AUDIOSTOP: // '"AudioStop" (the Stop media key)'
            return 0x23F9; // before: 0x40000104 (1073742084)
        case SDLK_AUDIOPLAY: // '"AudioPlay" (the Play media key)'
            return 0x23EF; // before: 0x40000105 (1073742085)
        case SDLK_AUDIOMUTE: // '"AudioMute" (the Mute volume key)'
            return 0x1F507; // before: 0x40000106 (1073742086)
        case SDLK_MEDIASELECT: // '"MediaSelect" (the Media Select key)'
            return 0x40000107; // 1073742087
        case SDLK_WWW: // '"WWW" (the WWW/World Wide Web key)'
            return 0x40000108; // 1073742088
        case SDLK_MAIL: // '"Mail" (the Mail/eMail key)'
            return 0x1F4E7; // before: 0x40000109 (1073742089)
        case SDLK_CALCULATOR: // '"Calculator" (the Calculator key)'
            return 0x4000010A; // 1073742090
        case SDLK_COMPUTER: // '"Computer" (the My Computer key)'
            return 0x1F5A5; // before: 0x4000010B (1073742091)
        case SDLK_AC_SEARCH: // '"AC Search" (the Search key (application control keypad))'
            return 0x1F50D; // before: 0x4000010C (1073742092)
        case SDLK_AC_HOME: // '"AC Home" (the Home key (application control keypad))'
            return 0x2302; // before: 0x4000010D (1073742093)
        case SDLK_AC_BACK: // '"AC Back" (the Back key (application control keypad))'
            return 0x4000010E; // 1073742094
        case SDLK_AC_FORWARD: // '"AC Forward" (the Forward key (application control keypad))'
            return 0x4000010F; // 1073742095
        case SDLK_AC_STOP: // '"AC Stop" (the Stop key (application control keypad))'
            return 0x40000110; // 1073742096
        case SDLK_AC_REFRESH: // '"AC Refresh" (the Refresh key (application control keypad))'
            return 0x27F3; // before: 0x40000111 (1073742097)
        case SDLK_AC_BOOKMARKS: // '"AC Bookmarks" (the Bookmarks key (application control keypad))'
            return 0x40000112; // 1073742098
        case SDLK_BRIGHTNESSDOWN: // '"BrightnessDown" (the Brightness Down key)'
            return 0x1F505; // before: 0x40000113 (1073742099)
        case SDLK_BRIGHTNESSUP: // '"BrightnessUp" (the Brightness Up key)'
            return 0x40000114; // 1073742100
        case SDLK_DISPLAYSWITCH: // '"DisplaySwitch" (display mirroring/dual display switch, video mode switch)'
            return 0x40000115; // 1073742101
        case SDLK_KBDILLUMTOGGLE: // '"KBDIllumToggle" (the Keyboard Illumination Toggle key)'
            return 0x40000116; // 1073742102
        case SDLK_KBDILLUMDOWN: // '"KBDIllumDown" (the Keyboard Illumination Down key)'
            return 0x40000117; // 1073742103
        case SDLK_KBDILLUMUP: // '"KBDIllumUp" (the Keyboard Illumination Up key)'
            return 0x40000118; // 1073742104
        case SDLK_EJECT: // '"Eject" (the Eject key)'
            return 0x23CF; // before: 0x40000119 (1073742105)
        case SDLK_SLEEP: // '"Sleep" (the Sleep key)'
            return 0x263E; // before: 0x4000011A (1073742106)
    }
    return 0;
}

Something keyCodeToChar(SDL_Keycode code){
    EmojicodeChar c = keyCodeToCharSwitch(code);
    if (c) {
        return somethingSymbol(c);
    }
    return NOTHINGNESS;
}
