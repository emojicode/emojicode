//
// Created by Theo Weidmann on 04.04.18.
//

#ifndef EMOJICODE_VTCREATOR_HPP
#define EMOJICODE_VTCREATOR_HPP

#include "Types/Class.hpp"

namespace EmojicodeCompiler {

class Declarator;

class VTCreator {
public:
    VTCreator(Class *klass, const Declarator &declarator)
            : declarator_(declarator), klass_(klass), hasSuperClass_(klass->superclass() != nullptr),
              vti_(hasSuperClass_ ? klass->superclass()->virtualFunctionCount() : 1) {}

    void assign();
    void build();
private:
    const Declarator &declarator_;
    Class *klass_;
    bool hasSuperClass_;
    size_t vti_;
    std::vector<llvm::Constant *> functions_;

    void assign(Function *reification);
};

}  // namespace EmojicodeCompiler

#endif //EMOJICODE_VTCREATOR_HPP
