//
//  ProtocolFunction.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 14/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ProtocolFunction_hpp
#define ProtocolFunction_hpp

#include "Function.hpp"

namespace EmojicodeCompiler {

/// ProtocolFunction instances are the methods that are added to Protocols. ProtocolFunction instance pretend to not
/// have a VTI until one was explicitely request and they ever enqueued for compilation. They do, however, propagate
/// @c assignVti and @c setUsed to their overriders.
class ProtocolFunction : public Function {
public:
    using Function::Function;
private:
    void assignVti() override;
    void setUsed(bool enqueue = true) override;
    bool assigned() const override { return assigned_; }
    bool assigned_ = false;
};

} // namespace EmojicodeCompiler

#endif /* ProtocolFunction_hpp */
