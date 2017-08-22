//
//  CommonTypeFinder.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CommonTypeFinder_hpp
#define CommonTypeFinder_hpp

#include "../Types/Type.hpp"
#include "../Lex/SourcePosition.hpp"
#include <vector>

namespace EmojicodeCompiler {

class CommonTypeFinder {
public:
    /// Tells the common type finder about the type of another element in the collection.
    void addType(const Type &type, const TypeContext &typeContext);
    /** Returns the common type and issues a warning at @c warningToken if the common type is ambigious. */
    Type getCommonType(const SourcePosition &p) const;
private:
    bool firstTypeFound_ = false;
    Type commonType_ = Type::something();
    std::vector<Type> commonProtocols_;
};

}  // namespace EmojicodeCompiler

#endif /* CommonTypeFinder_hpp */
