//
//  StringPool.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef StringPool_hpp
#define StringPool_hpp

#include <map>
#include <string>

namespace llvm {
class Value;
}  // namespace llvm

namespace EmojicodeCompiler {

class CodeGenerator;

/// StringPool represents an application’s string pool.
class StringPool {
public:
    explicit StringPool(CodeGenerator *cg) : codeGenerator_(cg) {}

    /// Pools the given string. The pool is searched for an identical strings first, if no such string is found,
    /// the string is added to the end of the pool.
    /// @returns The index to access the string in the pool.
    llvm::Value* pool(const std::u32string &string);
private:
    std::map<std::u32string, llvm::Value*> pool_;
    CodeGenerator *codeGenerator_;
};

}  // namespace EmojicodeCompiler

#endif /* StringPool_hpp */
