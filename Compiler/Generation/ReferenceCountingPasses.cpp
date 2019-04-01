//
//  ReferenceCountingPasses.cpp
//  runtime
//
//  Created by Theo Weidmann on 01.04.19.
//

#include "ReferenceCountingPasses.hpp"

namespace EmojicodeCompiler {

char ConstantReferenceCountingPass::id = 0;
char LocalReferenceCountingPass::id = 0;
char RedundantReferenceCountingPass::id = 0;

bool ReferenceCountingPass::runOnFunction(llvm::Function &function) {
    modified_ = false;
    for (auto &block : function) {
        for (auto &inst : block) {
            if (auto callInst = llvm::dyn_cast<llvm::CallInst>(&inst)) {
                if (isMemoryFunction(callInst->getCalledFunction())) {
                    transformMemoryInst(callInst);
                }
            }
        }
    }
    return modified_;
}

void ReferenceCountingPass::deleteInstructions() {
    for (auto inst : toBeDeleted_) {
        inst->eraseFromParent();
    }
    toBeDeleted_.clear();
}

bool ReferenceCountingPass::isMemoryFunction(llvm::Function *function) {
    return isRetainFunction(function) || isReleaseFunction(function);
}

bool ReferenceCountingPass::isRetainFunction(llvm::Function *function) {
    return function == runTime_->retainMemory() || function == runTime_->retain();
}

bool ReferenceCountingPass::isReleaseFunction(llvm::Function *function) {
    return function == runTime_->releaseMemory() || function == runTime_->release();
}

void LocalReferenceCountingPass::transformMemoryInst(llvm::CallInst *callInst) {
    auto operand = callInst->getArgOperand(0);

    auto bitcast = llvm::dyn_cast<llvm::BitCastInst>(operand);
    if (bitcast == nullptr) return;

    auto initCall = llvm::dyn_cast<llvm::CallInst>(bitcast->getOperand(0));

    if (initCall == nullptr || initCall->getCalledFunction() == nullptr ||
        initCall->getCalledFunction()->arg_size() == 0 ||
        !initCall->getCalledFunction()->hasParamAttribute(0, llvm::Attribute::Returned)) {
        return;
    }

    auto gep = llvm::dyn_cast<llvm::GetElementPtrInst>(initCall->getArgOperand(0));
    if (gep == nullptr || gep->getNumIndices() != 2) return;
    auto gepIdx1 = llvm::dyn_cast<llvm::ConstantInt>(gep->idx_begin());
    if (gepIdx1 == nullptr || !gepIdx1->isZero()) return;
    auto gepIdx2 = llvm::dyn_cast<llvm::ConstantInt>(gep->idx_begin() + 1);
    if (gepIdx2 == nullptr || !gepIdx2->isOne()) return;

    auto alloca = llvm::dyn_cast<llvm::AllocaInst>(gep->getPointerOperand());
    if (alloca == nullptr) return;

    if (callInst->getCalledFunction() == runTime_->release()) {
        callInst->setCalledFunction(runTime_->releaseLocal());
        modified_ = true;
    }
    else if (callInst->getCalledFunction() == runTime_->retain()) {
        llvm::IRBuilder<> builder(callInst);
        auto count = builder.CreateConstInBoundsGEP2_32(alloca->getType()->getElementType(), alloca, 0, 0);
        auto store = builder.CreateStore(builder.CreateAdd(builder.getInt64(1), builder.CreateLoad(count)), count);
        callInst->replaceAllUsesWith(store);
        modified_ = true;
    }
}

void ConstantReferenceCountingPass::transformMemoryInst(llvm::CallInst *callInst)  {
    auto operand = callInst->getArgOperand(0);
    if (llvm::dyn_cast<llvm::ConstantExpr>(operand) != nullptr) {
        toBeDeleted_.emplace_back(callInst);
        modified_ = true;
    }
}

void RedundantReferenceCountingPass::transformBlock(llvm::BasicBlock &block) {
    std::map<llvm::Value*, std::vector<llvm::CallInst*>> retains;
    for (auto &inst : block) {
        if (inst.getOpcode() == llvm::Instruction::Call) {
            auto callInst = llvm::dyn_cast<llvm::CallInst>(&inst);
            if (isRetainFunction(callInst->getCalledFunction())) {
                retains[callInst->getArgOperand(0)].emplace_back(callInst);
            }
            else if (!isReleaseFunction(callInst->getCalledFunction()) || !findCounterpart(callInst, retains)) {
                retains.clear();
            }
        }
    }
}

bool RedundantReferenceCountingPass::findCounterpart(llvm::CallInst *release,
                                                     std::map<llvm::Value*, std::vector<llvm::CallInst*>> &retains) {
    auto it = retains.find(release->getArgOperand(0));
    if (it == retains.end() || it->second.empty()) return false;
    toBeDeleted_.emplace_back(it->second.back());
    toBeDeleted_.emplace_back(release);
    it->second.pop_back();
    return true;
}

bool RedundantReferenceCountingPass::runOnFunction(llvm::Function &function) {
    modified_ = false;
    for (llvm::BasicBlock &block : function) {
        transformBlock(block);
    }
    deleteInstructions();
    return modified_;
}

}
