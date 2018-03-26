//
//  CommonTypeFinder.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CommonTypeFinder_hpp
#define CommonTypeFinder_hpp

#include "Types/Type.hpp"
#include <vector>

namespace EmojicodeCompiler {

class Compiler;
struct SourcePosition;

class CommonTypeFinder {
public:
    /// Tells the common type finder about the type of another element in the collection.
    void addType(const Type &type, const TypeContext &typeContext);
    /** Returns the common type and issues a warning at @c warningToken if the common type is ambigious. */
    Type getCommonType(const SourcePosition &p, Compiler *app) const;
private:
    void updateCommonProtocols(const Type &type, const TypeContext &typeContext);
    void updateCommonType(const Type &type, const TypeContext &typeContext);
    bool firstTypeFound_ = false;
    Type commonType_ = Type::something();
    std::vector<Type> commonProtocols_;

    void setCommonType(const Type &type);
};

}  // namespace EmojicodeCompiler

#endif /* CommonTypeFinder_hpp */
