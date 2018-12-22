//
// Created by Theo Weidmann on 04.04.18.
//

#include "VTCreator.hpp"
#include "Declarator.hpp"
#include "Functions/Function.hpp"

namespace EmojicodeCompiler {

void VTCreator::assign(const Reification<TypeDefinitionReification> *reifi, Function *function) {
    decltype(vti_) designatedVti;
    if (auto sf = hasSuperClass_ ? klass_->findSuperFunction(function) : nullptr) {
        designatedVti = sf->unspecificReification().vti();
    }
    else {
        functions_.emplace_back(nullptr);
        designatedVti = vti_++;
    }

    if (function->virtualTableThunk() != nullptr) {
        auto layer = function->virtualTableThunk();
        layer->createUnspecificReification();
        declarator_.declareLlvmFunction(layer, reifi);
        functions_[designatedVti] = layer->unspecificReification().function;

        functions_.emplace_back(function->unspecificReification().function);
        function->unspecificReification().setVti(designatedVti);
        vti_++;
    }
    else {
        function->unspecificReification().setVti(designatedVti);
        functions_[designatedVti] = function->unspecificReification().function;
    }
}

void VTCreator::build(Reification<TypeDefinitionReification> *reifi) {
    if (auto superclass = klass_->superclass()) {
        functions_.resize(superclass->virtualFunctionCount());
        std::copy(superclass->virtualTable().begin(), superclass->virtualTable().end(), functions_.begin());
    }

    assign(reifi);

    klass_->virtualTable() = std::move(functions_);
}

void VTCreator::assign(const Reification<TypeDefinitionReification> *reifi) {
    for (auto init : klass_->initializerList()) {
        init->createUnspecificReification();
        declarator_.declareLlvmFunction(init, reifi);
    }

    klass_->deinitializer()->createUnspecificReification();
    declarator_.declareLlvmFunction(klass_->deinitializer(), reifi);
    klass_->deinitializer()->unspecificReification().setVti(0);
    if (functions_.empty()) {
        functions_.emplace_back(klass_->deinitializer()->unspecificReification().function);
    }
    else {
        functions_[0] = klass_->deinitializer()->unspecificReification().function;
    }

    klass_->eachFunctionWithoutInitializers([&reifi, this](auto *function) {
        function->createUnspecificReification();
        if (function->unspecificReification().function == nullptr) {
            // If this is a super boxing layer it was already declared (see assign(Function*))
            declarator_.declareLlvmFunction(function, reifi);
        }

        if (function->accessLevel() == AccessLevel::Private) {
            return;
        }

        assign(reifi, function);
    });
    klass_->setVirtualFunctionCount(vti_);
}

}  // namespace EmojicodeCompiler
