//
// Created by Theo Weidmann on 04.04.18.
//

#ifndef EMOJICODE_VTCREATOR_HPP
#define EMOJICODE_VTCREATOR_HPP

#include "Types/Class.hpp"

namespace EmojicodeCompiler {

class Declarator;

/// VTCreator is responsible for creating the virtual table for a class.
class VTCreator {
public:
    /// @param klass The class for which the virtual table should be created.
    VTCreator(Class *klass, const Declarator &declarator)
            : declarator_(declarator), klass_(klass), hasSuperClass_(klass->superclass() != nullptr),
              vti_(hasSuperClass_ ? klass->superclass()->virtualFunctionCount() : 1) {}

    /// Assings VTI’s to the methods of the class and assigns the generated table to Class::virtualTable.
    void build(Reification<TypeDefinitionReification> *reifi);

private:
    void assign(const Reification<TypeDefinitionReification> *reifi);
    const Declarator &declarator_;
    Class *klass_;
    bool hasSuperClass_;
    size_t vti_;
    std::vector<llvm::Constant *> functions_;

    void assign(const Reification<TypeDefinitionReification> *reifi, Function *reification);
};

}  // namespace EmojicodeCompiler

#endif //EMOJICODE_VTCREATOR_HPP
