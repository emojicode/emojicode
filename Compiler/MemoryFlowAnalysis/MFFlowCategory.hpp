//
//  MFFlowCategory.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 31/05/2018.
//  Copyright © 2018 Theo Weidmann. All rights reserved.
//

#ifndef MFType_hpp
#define MFType_hpp

namespace EmojicodeCompiler {

/// The flow categories categorizing the use of a value.
/// @see MFFunctionAnalyser for a detailed explanation.
enum class MFFlowCategory {
    Borrowing, Escaping, Unknown
};

}  // namespace EmojicodeCompiler

#endif /* MFType_hpp */
