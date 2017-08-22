//
//  STIProvider.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 14/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef STIProvider_hpp
#define STIProvider_hpp

#include "VTIProvider.hpp"

namespace EmojicodeCompiler {

/// The STIProvider is used for static virtual function dispatch, i.e. value types methods, closures, the start flag.
class STIProvider : public VTIProvider {
public:
    /// The global STIProvider is used for functions created by the compiler that do not belong to any type. E.g. the
    /// start flag function, closures.
    static STIProvider globalStiProvider;
    /// Returns the number of all static virtual functions.
    static unsigned int functionCount() { return nextVti_; }

    int next() override;
protected:
    STIProvider() = default;
private:
    static int nextVti_;
};

class ValueTypeVTIProvider : public STIProvider {
public:
    ValueTypeVTIProvider() = default;
};

} // namespace EmojicodeCompiler

#endif /* STIProvider_hpp */
