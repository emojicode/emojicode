//
// Created by Theo Weidmann on 04.04.18.
//

#include "VTCreator.hpp"
#include "Declarator.hpp"
#include "Functions/Function.hpp"

namespace EmojicodeCompiler {

void VTCreator::assign(Function *function) {
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
        declarator_.declareLlvmFunction(layer);
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

void VTCreator::build() {
    if (auto superclass = klass_->superclass()) {
        functions_.resize(superclass->virtualFunctionCount());
        std::copy(superclass->virtualTable().begin(), superclass->virtualTable().end(), functions_.begin());
    }

    assign();

    klass_->virtualTable() = std::move(functions_);
}

void VTCreator::assign() {
    for (auto init : klass_->initializerList()) {
        init->createUnspecificReification();
        declarator_.declareLlvmFunction(init);
    }
    klass_->eachFunctionWithoutInitializers([this](auto *function) {
        function->createUnspecificReification();
        if (function->unspecificReification().function == nullptr) {
            // If this is a super boxing layer it was already declared (see assign(Function*))
            declarator_.declareLlvmFunction(function);
        }

        if (function->accessLevel() == AccessLevel::Private) {
            return;
        }

        assign(function);
    });
    klass_->setVirtualFunctionCount(vti_);
}

}  // namespace EmojicodeCompiler
