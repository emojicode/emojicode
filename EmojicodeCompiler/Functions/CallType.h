//
//  CallType.h
//  Emojicode
//
//  Created by Theo Weidmann on 04/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef CallType_h
#define CallType_h

namespace EmojicodeCompiler {

enum class CallType {
    None, DynamicDispatch, StaticDispatch, StaticContextfreeDispatch, DynamicProtocolDispatch,
};

}  // namespace EmojicodeCompiler

#endif /* CallType_h */
