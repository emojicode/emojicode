//
//  RunTimeTypeInfoFlags.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 23.03.19.
//

#ifndef GenericTypeInfoFlags_h
#define GenericTypeInfoFlags_h

namespace EmojicodeCompiler {

struct RunTimeTypeInfoFlags {

    enum Flags : int8_t {
        Class = 0, ValueType = 1, ValueTypeRemote = 2, Protocol = 3,
        /// For Something and Someobject
        Abstract = 4,
        // TODO: Multiprotocol, Callable
    };
};

}

#endif /* GenericTypeInfoFlags_h */
