//
//  Mood.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 30.11.18.
//

#ifndef Mood_h
#define Mood_h

#include <string>

namespace EmojicodeCompiler {

enum class Mood {
    Imperative,
    Interogative,
    Assignment,
};

/// Returns the emoji that is used to declare a method with the specified mood.
inline const char* moodEmoji(Mood mood) {
    switch (mood) {
        case Mood::Interogative:
            return "❓";
        case Mood::Imperative:
            return "❗️";
        case Mood::Assignment:
            return "➡️";
    }
}

}  // namespace EmojicodeCompiler

#endif /* Mood_h */
