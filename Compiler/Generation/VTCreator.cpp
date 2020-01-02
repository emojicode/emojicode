//
// Created by Theo Weidmann on 04.04.18.
//

#include "VTCreator.hpp"
#include "Types/Class.hpp"
#include "CodeGenerator.hpp"
#include "Functions/Function.hpp"

namespace EmojicodeCompiler {

VTCreator::VTCreator(Class *klass, CodeGenerator *cg)
    : generator_(cg), klass_(klass),
     vti_(klass->superclass() != nullptr ? klass->superclass()->virtualFunctionCount() : 0) {}

void VTCreator::assign(Function *function) {
    decltype(vti_) designatedVti;
    if (auto sf = function->superFunction()) {
        designatedVti = sf->unspecificReification().vti();
    }
    else {
        functions_.emplace_back(nullptr);
        designatedVti = vti_++;
    }

    if (function->virtualTableThunk() != nullptr) {
        auto layer = function->virtualTableThunk();
        layer->createUnspecificReification();
        generator_->declareLlvmFunction(layer);
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
    for (auto init : klass_->inits().list()) {
        init->createUnspecificReification();
        generator_->declareLlvmFunction(init);
    }

    klass_->eachFunctionWithoutInitializers([this](auto *function) {
        function->createUnspecificReification();
        if (function->unspecificReification().function == nullptr) {
            // If this is a super boxing layer it was already declared (see assign(Function*))
            generator_->declareLlvmFunction(function);
        }

        if (function->accessLevel() == AccessLevel::Private ||
            function->functionType() == FunctionType::Deinitializer) {
            return;
        }

        assign(function);
    });
    klass_->setVirtualFunctionCount(vti_);
}

}  // namespace EmojicodeCompiler
