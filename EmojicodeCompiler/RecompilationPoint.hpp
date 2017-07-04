//
//  RecompilationPoint.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 17/02/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef RecompilationPoint_hpp
#define RecompilationPoint_hpp

#include "FunctionWriter.hpp"
#include "TokenStream.hpp"

namespace EmojicodeCompiler {

/// Allows the state of the token stream and the callable writer to be stored, to which they will be restored upon
/// request. This is helpful when part of the code needs to be recompiled under different settings.
class RecompilationPoint {
public:
    RecompilationPoint(FunctionWriter &writer, TokenStream &stream)
    : writerSize_(writer.instructions_.size()), writer_(writer), index_(stream.index_), stream_(stream) {}
    /// Restores the token stream and callable writer to the state they had when the RecompilationPoint was created.
    /// @warning All FunctionWriterInsertionPoints and FunctionWriterPlaceholders created after the creation of the
    /// RecompilationPoint are invalidated!
    void restore() const {
        stream_.index_ = index_;
        writer_.instructions_.resize(writerSize_);
    }
private:
    size_t writerSize_;
    FunctionWriter &writer_;
    size_t index_;
    TokenStream &stream_;
};

};  // namespace EmojicodeCompiler

#endif /* RecompilationPoint_hpp */
