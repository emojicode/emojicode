//
//  Initializer.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 22/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Initializer_hpp
#define Initializer_hpp

#include "Function.hpp"
#include <algorithm>

namespace EmojicodeCompiler {

class Initializer final : public Function {
public:
    Initializer(std::u32string name, AccessLevel level, bool final, TypeDefinition *typeDef, Package *package,
                SourcePosition p, bool overriding, std::u32string documentationToken, bool deprecated, bool r,
                bool unsafe, FunctionType mode)
    : Function(std::move(name), level, final, typeDef, package, std::move(p), overriding,
               std::move(documentationToken), deprecated, true, true, unsafe, mode), required_(r) {}

    /// Whether all subclassess are required to implement this initializer as well. Never true for non-class types.
    bool required() const { return required_; }
    /// Whether this initializer might return an error.
    bool errorProne() const { return errorType_ != nullptr; }
    ASTType* errorType() const { return errorType_.get(); }

    void setErrorType(std::unique_ptr<ASTType> type) { errorType_ = std::move(type); }

    /// Returns the actual type constructed with this initializer for the given initialized type @c type
    Type constructedType(Type type) const {
        type = type.unboxed();
        if (errorProne()) {
            return type.errored(errorType_->type());
        }
        return type;
    }
    void addArgumentToVariable(const std::u32string &string, const SourcePosition &p) {
        auto find = std::find(argumentsToVariables_.begin(), argumentsToVariables_.end(), string);
        if (find != argumentsToVariables_.end()) {
            throw CompilerError(p, "Instance variable initialized with 🍼 more than once.");
        }
        argumentsToVariables_.push_back(string);
    }
    const std::vector<std::u32string>& argumentsToVariables() const { return argumentsToVariables_; }
private:
    bool required_;
    std::unique_ptr<ASTType> errorType_ = nullptr;
    std::vector<std::u32string> argumentsToVariables_;
};

}  // namespace EmojicodeCompiler

#endif /* Initializer_hpp */
