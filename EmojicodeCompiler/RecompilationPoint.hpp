//
//  RecompilationPoint.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 17/02/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef RecompilationPoint_hpp
#define RecompilationPoint_hpp

#include "CallableWriter.hpp"
#include "TokenStream.hpp"

/// Allows the state of the token stream and the callable writer to be stored, to which they will be restored upon
/// request. This is helpful when part of the code needs to be recompiled under different settings.
class RecompilationPoint {
public:
    RecompilationPoint(CallableWriter &writer, TokenStream &stream)
    : writerSize_(writer.instructions_.size()), writer_(writer), token_(stream.currentToken_), stream_(stream) {}
    /// Restores the token stream and callable writer to the state they had when the RecompilationPoint was created.
    /// @warning All CallableWriterInsertionPoints and CallableWriterPlaceholders created after the creation of the
    /// RecompilationPoint are invalidated!
    void restore() const {
        stream_.currentToken_ = token_;
        writer_.instructions_.resize(writerSize_);
    }
private:
    size_t writerSize_;
    CallableWriter &writer_;
    const Token *token_;
    TokenStream &stream_;
};

#endif /* RecompilationPoint_hpp */
