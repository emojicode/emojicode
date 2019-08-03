//
//  MFFlowCategory.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 31/05/2018.
//  Copyright © 2018 Theo Weidmann. All rights reserved.
//

#include <cassert>

#ifndef MFType_hpp
#define MFType_hpp

namespace EmojicodeCompiler {

/// The flow categories categorizing the use of a value.
/// @see MFFunctionAnalyser for a detailed explanation.
class MFFlowCategory {
    enum class Category {
        Borrowing, Escaping, Return, Unknown
    };

public:
    MFFlowCategory() : MFFlowCategory(Category::Unknown) {}

    static const MFFlowCategory Borrowing;
    static const MFFlowCategory Escaping;
    static const MFFlowCategory Return;

    bool isEscaping() const {
        assert(!isUnknown());
        return category_ == Category::Return || category_ == Category::Escaping;
    }

    bool isUnknown() const { return category_ == Category::Unknown; }

    bool isReturn() const { return category_ == Category::Return; }

    bool fulfillsPromise(MFFlowCategory promise) const {
        return category_ == Category::Borrowing || promise.isEscaping();
    }

private:
    MFFlowCategory(Category c) : category_(c) {}

    Category category_;
};

}  // namespace EmojicodeCompiler

#endif /* MFType_hpp */
