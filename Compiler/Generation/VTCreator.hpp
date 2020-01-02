//
// Created by Theo Weidmann on 04.04.18.
//

#ifndef EMOJICODE_VTCREATOR_HPP
#define EMOJICODE_VTCREATOR_HPP

#include <cstddef>
#include <vector>

namespace llvm {
class Constant;
} // namespace llvm

namespace EmojicodeCompiler {

class CodeGenerator;
class Class;
class Function;

/// VTCreator is responsible for creating the virtual table for a class.
class VTCreator {
public:
    /// @param klass The class for which the virtual table should be created.
    VTCreator(Class *klass, CodeGenerator *cg);

    /// Assigns VTI’s to the methods of the class and assigns the generated table to Class::virtualTable.
    void build();

private:
    void assign();
    CodeGenerator *generator_;
    Class *klass_;
    size_t vti_;
    std::vector<llvm::Constant *> functions_;

    void assign(Function *reification);
};

}  // namespace EmojicodeCompiler

#endif //EMOJICODE_VTCREATOR_HPP
