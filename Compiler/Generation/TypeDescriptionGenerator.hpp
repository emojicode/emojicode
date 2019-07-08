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

enum class TypeDescriptionUser {
    Class, ValueTypeOrValue, Function,
};

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
    using User = TypeDescriptionUser;

    /// Creates an instance. Each instance can only be used to generate one description.
    TypeDescriptionGenerator(FunctionCodeGenerator *fg, User user) : fg_(fg), user_(user) {}

    llvm::Value* generate(const std::vector<Type> &types);
    llvm::Value* generate(const Type &type);
    llvm::Value* generate(const std::vector<std::shared_ptr<ASTType>> &types);

    /// Must be called when User is User::Function, after the called function has returned.
    void restoreStack();

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
    /// Whether the description is for a class
    User user_;

    llvm::Value* extractTypeDescriptionPtr();

    llvm::Value *stack_ = nullptr;
};

}

#endif /* TypeDescriptionGenerator_hpp */
