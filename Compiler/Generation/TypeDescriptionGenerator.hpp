//
//  TypeDescriptionGenerator.hpp
//  runtime
//
//  Created by Theo Weidmann on 25.03.19.
//

#ifndef TypeDescriptionGenerator_hpp
#define TypeDescriptionGenerator_hpp

#include <vector>
#include <memory>

namespace llvm {
class Value;
class Constant;
}

namespace EmojicodeCompiler {

class FunctionCodeGenerator;
class Type;
class ASTType;

/// The TypeDescriptionGenerator creates a %typeDescription* pointing to the first element of an array describing one or
/// more types and their generic arguments.
///
/// If none the provided types requires dynamism (i.e. none is a generic variable) the array is created as a global
/// variable.
class TypeDescriptionGenerator {
    struct TypeDescriptionValue {
        TypeDescriptionValue(llvm::Constant *constant) : concrete(constant) {}
        TypeDescriptionValue(llvm::Value *from, llvm::Value *size) : from(from), size(size) {}
        llvm::Constant *concrete = nullptr;
        llvm::Value *from;
        llvm::Value *size;

        bool isCopy() const { return concrete == nullptr; }
    };

public:
    /// Creates an instance. Each instance can only be used to generate one description.
    TypeDescriptionGenerator(FunctionCodeGenerator *fg) : fg_(fg) {}

    llvm::Value* generate(const std::vector<Type> &types);
    llvm::Value* generate(const Type &type);
    llvm::Value* generate(const std::vector<std::shared_ptr<ASTType>> &types);

private:
    void addType(const Type &type);
    llvm::Value* finish();
    llvm::Value* finishStatic();
    void addDynamic(llvm::Value *gargs, size_t index);

    FunctionCodeGenerator *fg_;
    /// Describes all values that will appear in the array
    std::vector<TypeDescriptionValue> types_;
    /// Counts the type descriptions that are dynamic, i.e. copied from either the local or type generic arguments
    unsigned int dynamic_ = 0;
};

}

#endif /* TypeDescriptionGenerator_hpp */
