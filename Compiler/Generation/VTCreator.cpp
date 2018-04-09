//
// Created by Theo Weidmann on 04.04.18.
//

#include "VTCreator.hpp"
#include "Declarator.hpp"
#include "Functions/Function.hpp"

namespace EmojicodeCompiler {

void VTCreator::assign(Function *function) {
    if (function->superBoxingLayer() != nullptr) {
        auto vti = klass_->findSuperFunction(function)->unspecificReification().vti();
        auto layer = function->superBoxingLayer();
        layer->createUnspecificReification();
        layer->unspecificReification().setVti(vti);
        declarator_.declareLlvmFunction(layer);
        functions_[vti] = function->superBoxingLayer()->unspecificReification().function;
    }
    else if (auto sf = hasSuperClass_ ? klass_->findSuperFunction(function) : nullptr) {
        auto vti = sf->unspecificReification().vti();
        function->unspecificReification().setVti(vti);
        functions_[vti] = function->unspecificReification().function;
        return;
    }

    function->unspecificReification().setVti(vti_++);
    functions_.emplace_back(function->unspecificReification().function);
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
    klass_->eachFunction([this](auto *function) {
        function->createUnspecificReification();
        if (function->unspecificReification().function == nullptr) {
            // If this is a super boxing layer it was already declared
            declarator_.declareLlvmFunction(function);
        }

        if (function->accessLevel() == AccessLevel::Private) {
            return;
        }

        if (function->functionType() == FunctionType::ObjectInitializer) {
            auto initializer = dynamic_cast<Initializer *>(function);
            if (!initializer->required()) {
                return;
            }
            assign(initializer);
        }
        else {
            assign(function);
        }
    });
    klass_->setVirtualFunctionCount(vti_);
}

}
