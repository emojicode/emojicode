//
//  BoxRetainReleaseBuilder.cpp
//  runtime
//
//  Created by Theo Weidmann on 30.03.19.
//

#include "BoxRetainReleaseBuilder.hpp"
#include "Types/Type.hpp"
#include "Types/TypeContext.hpp"
#include "Types/ValueType.hpp"
#include "Types/Class.hpp"
#include "Scoping/Scope.hpp"
#include "FunctionCodeGenerator.hpp"
#include "CallCodeGenerator.hpp"
#include "CodeGenerator.hpp"
#include "Mangler.hpp"
#include "RunTimeHelper.hpp"
#include "AST/ASTExpr.hpp"

namespace EmojicodeCompiler {

llvm::Function* createFunction(CodeGenerator *cg, const std::string &name) {
    auto fn = llvm::Function::Create(cg->typeHelper().boxRetainRelease(),
                                     llvm::GlobalValue::LinkageTypes::PrivateLinkage, name,
                                     cg->module());
    fn->setUnnamedAddr(llvm::GlobalVariable::UnnamedAddr::Global);
    return fn;
}

llvm::Function* createMemoryFunction(const std::string &str, CodeGenerator *cg, TypeDefinition *typeDef) {
    auto type = typeDef->type().is<TypeType::ValueType>() ? typeDef->type().referenced() : typeDef->type();
    auto ft = llvm::FunctionType::get(llvm::Type::getVoidTy(cg->context()), llvm::ArrayRef<llvm::Type*>{ cg->typeHelper().llvmTypeFor(type) }, false);
    auto fn = llvm::Function::Create(ft, llvm::GlobalValue::LinkageTypes::ExternalLinkage, str, cg->module());
    fn->setUnnamedAddr(llvm::GlobalVariable::UnnamedAddr::Global);
    return fn;
}

void buildCopyRetain(CodeGenerator *cg, ValueType *typeDef) {
    FunctionCodeGenerator fg(typeDef->copyRetain(), cg, std::make_unique<TypeContext>(Type(typeDef)));
    fg.createEntry();
    for (auto &decl : typeDef->instanceVariables()) {
        auto &var = typeDef->instanceScope().getLocalVariable(decl.name);
        if (var.type().isManaged()) {
            auto ptr = fg.instanceVariablePointer(var.id());
            if (!fg.isManagedByReference(var.type())) {
                ptr = fg.builder().CreateLoad(ptr);
            }
            fg.retain(ptr, var.type());
        }
    }

    if (typeDef->storesGenericArgs()) {
        auto opc = fg.builder().CreateBitCast(fg.builder().CreateLoad(fg.genericArgsPtr()),
                                              llvm::Type::getInt8PtrTy(fg.ctx()));
        fg.builder().CreateCall(fg.generator()->runTime().retain(), { opc });
    }
    fg.builder().CreateRetVoid();
}

void buildDestructor(CodeGenerator *cg, TypeDefinition *typeDef) {
    FunctionCodeGenerator fg(typeDef->destructor(), cg, std::make_unique<TypeContext>(typeDef->type()));
    fg.createEntry();
    auto klass = dynamic_cast<Class *>(typeDef);
    if (klass != nullptr && klass->deinitializer() != nullptr) {
        CallCodeGenerator ccg(&fg, CallType::StaticDispatch);
        for (auto aKlass = klass; aKlass != nullptr; aKlass = aKlass->superclass()) {
            if (auto deinit = aKlass->deinitializer()) {
                ccg.generate(fg.thisValue(), fg.calleeType(), ASTArguments(typeDef->position()), deinit, nullptr);
            }
        }
    }

    for (auto it = typeDef->instanceVariables().rbegin(); it < typeDef->instanceVariables().rend(); it++) {
        auto &decl = *it;
        auto &var = typeDef->instanceScope().getLocalVariable(decl.name);
        if (var.type().isManaged()) {
            fg.releaseByReference(fg.instanceVariablePointer(var.id()), var.type());
        }
    }

    if (typeDef->storesGenericArgs()) {
        auto val = fg.builder().CreateLoad(fg.genericArgsPtr());
        if (fg.calleeType().is<TypeType::ValueType>()) {
            auto opc = fg.builder().CreateBitCast(val, llvm::Type::getInt8PtrTy(fg.ctx()));
            fg.builder().CreateCall(fg.generator()->runTime().releaseMemory(), { opc });
        }
        else {
            fg.createIf(fg.builder().CreateIsNull(fg.builder().CreateExtractValue(val, { 1 })), [&] {
                auto opc = fg.builder().CreateBitCast(fg.builder().CreateExtractValue(val, { 0 }),
                                                      llvm::Type::getInt8PtrTy(fg.ctx()));
                fg.builder().CreateCall(fg.generator()->runTime().free(), { opc });
            });
        }
    }
    fg.builder().CreateRetVoid();
}


std::pair<llvm::Function*, llvm::Function*> buildBoxRetainRelease(CodeGenerator *cg, const Type &type) {
    auto release = createFunction(cg, mangleBoxRelease(type));
    auto retain = createFunction(cg, mangleBoxRetain(type));

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
