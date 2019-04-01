//
//  BoxRetainReleaseBuilder.cpp
//  runtime
//
//  Created by Theo Weidmann on 30.03.19.
//

#include "BoxRetainReleaseBuilder.hpp"
#include "Types/Type.hpp"
#include "Types/TypeContext.hpp"
#include "FunctionCodeGenerator.hpp"
#include "CodeGenerator.hpp"
#include "Mangler.hpp"
#include "RunTimeHelper.hpp"

namespace EmojicodeCompiler {

std::pair<llvm::Function*, llvm::Function*> buildBoxRetainRelease(CodeGenerator *cg, const Type &type) {
    auto release = llvm::Function::Create(cg->typeHelper().boxRetainRelease(),
                                          llvm::GlobalValue::LinkageTypes::PrivateLinkage, mangleBoxRelease(type),
                                          cg->module());
    auto retain = llvm::Function::Create(cg->typeHelper().boxRetainRelease(),
                                         llvm::GlobalValue::LinkageTypes::PrivateLinkage, mangleBoxRetain(type),
                                         cg->module());
    release->setUnnamedAddr(llvm::GlobalVariable::UnnamedAddr::Global);
    retain->setUnnamedAddr(llvm::GlobalVariable::UnnamedAddr::Global);

    FunctionCodeGenerator releaseFg(release, cg, std::make_unique<TypeContext>(type));
    releaseFg.createEntry();
    FunctionCodeGenerator retainFg(retain, cg, std::make_unique<TypeContext>(type));
    retainFg.createEntry();

    if (type.isManaged()) {
        if (!releaseFg.isManagedByReference(type)) {
            auto objPtr = releaseFg.buildGetBoxValuePtr(release->args().begin(), type);
            releaseFg.release(releaseFg.builder().CreateLoad(objPtr), type);

            auto objPtrRetain = retainFg.buildGetBoxValuePtr(retain->args().begin(), type);
            retainFg.retain(retainFg.builder().CreateLoad(objPtrRetain), type);
        }
        else if (cg->typeHelper().isRemote(type)) {
            auto containedType = cg->typeHelper().llvmTypeFor(type);
            auto mngType = cg->typeHelper().managable(containedType);

            auto objPtr = releaseFg.buildGetBoxValuePtrAfter(release->args().begin(), mngType->getPointerTo(),
                                                             containedType->getPointerTo());
            auto remotePtr = releaseFg.builder().CreateLoad(objPtr);
            releaseFg.release(releaseFg.managableGetValuePtr(remotePtr), type);
            releaseFg.builder().CreateCall(cg->runTime().releaseWithoutDeinit(),
                                           releaseFg.builder().CreateBitCast(remotePtr,
                                                                             llvm::Type::getInt8PtrTy(cg->context())));

            auto objPtrRetain = retainFg.buildGetBoxValuePtrAfter(retain->args().begin(), mngType->getPointerTo(),
                                                                  containedType->getPointerTo());
            auto remotePtrRetain = retainFg.builder().CreateLoad(objPtrRetain);
            retainFg.retain(retainFg.managableGetValuePtr(remotePtrRetain), type);
            retainFg.builder().CreateCall(cg->runTime().retain(),
                                          retainFg.builder().CreateBitCast(remotePtrRetain,
                                                                           llvm::Type::getInt8PtrTy(cg->context())));
        }
        else {
            auto objPtr = releaseFg.buildGetBoxValuePtr(release->args().begin(), type);
            releaseFg.release(objPtr, type);

            auto objPtrRetain = retainFg.buildGetBoxValuePtr(retain->args().begin(), type);
            retainFg.retain(objPtrRetain, type);
        }
    }

    releaseFg.builder().CreateRetVoid();
    retainFg.builder().CreateRetVoid();
    return std::make_pair(retain, release);
}

}
