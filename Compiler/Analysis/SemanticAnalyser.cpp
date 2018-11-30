//
// Created by Theo Weidmann on 26.02.18.
//

#include "SemanticAnalyser.hpp"
#include "Compiler.hpp"
#include "DeinitializerBuilder.hpp"
#include "FunctionAnalyser.hpp"
#include "AST/ASTExpr.hpp"
#include "Scoping/SemanticScoper.hpp"
#include "Types/TypeExpectation.hpp"
#include "Package/Package.hpp"
#include "ThunkBuilder.hpp"
#include "Types/Class.hpp"
#include "Types/Extension.hpp"
#include "Types/Protocol.hpp"
#include "Types/TypeDefinition.hpp"
#include "Types/ValueType.hpp"

namespace EmojicodeCompiler {

Compiler* SemanticAnalyser::compiler() const {
    return package_->compiler();
}

void SemanticAnalyser::analyse(bool executable) {
    for (auto &extension : package_->extensions()) {
        extension->extend();
    }

    for (auto &vt : package_->valueTypes()) {
        finalizeProtocols(Type(vt.get()));
        vt->analyseConstraints(TypeContext(Type(vt.get())));
    }
    for (auto &klass : package_->classes()) {
        klass->analyseSuperType();
        finalizeProtocols(Type(klass.get()));
        klass->analyseConstraints(TypeContext(Type(klass.get())));
    }

    package_->recreateClassTypes();

    // Now all types are ready to be used with compatibleTo

    for (auto &protocol : package_->protocols()) {
        protocol->eachFunction([this](Function *function) {
            analyseFunctionDeclaration(function);
        });
    }
    for (auto &vt : package_->valueTypes()) {
        checkProtocolConformance(Type(vt.get()));
        declareInstanceVariables(Type(vt.get()));
        buildDeinitializer(vt.get());
        buildCopyRetain(vt.get());
        enqueueFunction(vt->deinitializer());
        enqueueFunction(vt->copyRetain());
        enqueueFunctionsOfTypeDefinition(vt.get());
    }
    for (auto &klass : package_->classes()) {
        for (auto init : klass->initializerList()) {
            if (!init->required()) {
                continue;
            }
            klass->addTypeMethod(buildRequiredInitThunk(klass.get(), init));
        }
        
        klass->inherit(this);
        checkProtocolConformance(Type(klass.get()));

        buildDeinitializer(klass.get());
        enqueueFunction(klass->deinitializer());

        enqueueFunctionsOfTypeDefinition(klass.get());

        if (!klass->hasSubclass() && !klass->exported()) {
            klass->setFinal();
        }
    }
    for (auto &function : package_->functions()) {
        enqueueFunction(function.get());
    }

    analyseQueue();
    checkStartFlagFunction(executable);
}

void SemanticAnalyser::checkStartFlagFunction(bool executable) {
    if (package_->hasStartFlagFunction()) {
        auto returnType = package_->startFlagFunction()->returnType()->type();
        if (returnType.type() != TypeType::NoReturn &&
            !returnType.compatibleTo(Type(package_->compiler()->sInteger), TypeContext())) {
            package_->compiler()->error(CompilerError(package_->startFlagFunction()->position(),
                                                      "🏁 must either have no return or return 🔢."));
        }
        if (!executable) {
            package_->startFlagFunction()->makeExternal();  // Prevent function from being included in object file
        }
    }
    else if (executable) {
        compiler()->error(CompilerError(SourcePosition(), "No 🏁 block was found."));
    }
}

void SemanticAnalyser::analyseQueue() {
    while (!queue_.empty()) {
        try {
            FunctionAnalyser(queue_.front(), this).analyse();
        }
        catch (CompilerError &ce) {
            package_->compiler()->error(ce);
        }
        queue_.pop();
    }
}

void SemanticAnalyser::enqueueFunctionsOfTypeDefinition(TypeDefinition *typeDef) {
    typeDef->eachFunction([this](Function *function) {
        enqueueFunction(function);
    });
}

void SemanticAnalyser::enqueueFunction(Function *function) {
    analyseFunctionDeclaration(function);
    if (!function->isExternal()) {
        queue_.emplace(function);
    }
}

void SemanticAnalyser::analyseFunctionDeclaration(Function *function) const {
    if (function->returnType() == nullptr) {
        function->setReturnType(std::make_unique<ASTLiteralType>(Type::noReturn()));
    }
    else if (function->returnType()->wasAnalysed()) {
        return;
    }

    auto context = function->typeContext();

    function->analyseConstraints(context);
    for (auto &param : function->parameters()) {
        param.type->analyseType(context);
        if (!function->externalName().empty() && param.type->type().type() == TypeType::ValueType &&
            !param.type->type().valueType()->isPrimitive()) {
            param.type->type().setReference();
        }
    }

    if (auto initializer = dynamic_cast<Initializer*>(function)) {
        if (initializer->errorProne() && initializer->errorType()->analyseType(context).type() != TypeType::Enum) {
            throw CompilerError(initializer->errorType()->position(), "Error type must be a non-optional 🦃.");
        }
        return;
    }
    function->returnType()->analyseType(context);
}

void SemanticAnalyser::declareInstanceVariables(const Type &type) {
    TypeDefinition *typeDef = type.typeDefinition();

    auto context = TypeContext(type);
    auto scoper = std::make_unique<SemanticScoper>();
    scoper->pushScope();  // For closure analysis
    ExpressionAnalyser analyser(this, context, package_, std::move(scoper));

    for (auto &var : typeDef->instanceVariables()) {
        typeDef->instanceScope().declareVariable(var.name, var.type->analyseType(context), false,
                                                 var.position);

        if (var.expr != nullptr) {
            var.expr->analyse(&analyser, TypeExpectation(var.type->type()));
        }
    }

    if (!typeDef->instanceVariables().empty() && typeDef->initializerList().empty()) {
        package_->compiler()->warn(typeDef->position(), "Type defines ", typeDef->instanceVariables().size(),
                                   " instances variables but has no initializers.");
    }
}

bool SemanticAnalyser::checkReturnPromise(const Function *sub, const TypeContext &subContext,
                                          const Function *super, const TypeContext &superContext,
                                          const Type &superSource) const {
    auto superReturnType = super->returnType()->type().resolveOn(superContext);
    if (!sub->returnType()->type().resolveOn(subContext).compatibleTo(superReturnType, subContext)) {
        auto supername = superReturnType.toString(subContext);
        auto thisname = sub->returnType()->type().toString(subContext);
        package_->compiler()->error(CompilerError(sub->position(), "Return type ",
                                                  sub->returnType()->type().toString(subContext), " of ",
                                                  utf8(sub->name()),
                                                  " is not compatible to the return type defined in ",
                                                  superSource.toString(subContext)));
    }
    return sub->returnType()->type().resolveOn(subContext).storageType() == superReturnType.storageType();
}

std::unique_ptr<Function> SemanticAnalyser::enforcePromises(Function *sub, Function *super,
                                                            const Type &superSource,
                                                            const TypeContext &subContext,
                                                            const TypeContext &superContext) {
    analyseFunctionDeclaration(sub);
    analyseFunctionDeclaration(super);
    if (super->final()) {
        package_->compiler()->error(CompilerError(sub->position(), superSource.toString(subContext),
                                                  "’s implementation of ", utf8(sub->name()), " was marked 🔏."));
    }
    if (sub->accessLevel() == AccessLevel::Private || (sub->accessLevel() == AccessLevel::Protected &&
            super->accessLevel() == AccessLevel::Public)) {
        package_->compiler()->error(CompilerError(sub->position(), "Overriding method must be as accessible or more ",
                                                  "accessible than the overridden method."));
    }

    bool isReturnOk = checkReturnPromise(sub, subContext, super, superContext, superSource);
    bool isParamsOk = checkArgumentPromise(sub, super, subContext, superContext) ;
    if (!isParamsOk || !isReturnOk) {
        return buildBoxingThunk(superContext, super, sub);
    }
    return nullptr;
}

bool SemanticAnalyser::checkArgumentPromise(const Function *sub, const Function *super, const TypeContext &subContext,
                                            const TypeContext &superContext) const {
    if (super->parameters().size() != sub->parameters().size()) {
        package_->compiler()->error(CompilerError(sub->position(), "Parameter count does not match."));
        return true;
    }

    bool compatible = true;
    for (size_t i = 0; i < super->parameters().size(); i++) { // More general arguments are OK
        auto superArgumentType = super->parameters()[i].type->type().resolveOn(superContext);
        if (!superArgumentType.compatibleTo(sub->parameters()[i].type->type().resolveOn(subContext), subContext)) {
            auto supertype = superArgumentType.toString(subContext);
            auto thisname = sub->parameters()[i].type->type().toString(subContext);
            package_->compiler()->error(CompilerError(sub->position(), "Type ", thisname, " of argument ", i + 1,
                                                      " is not compatible with its ", thisname, " argument type ",
                                                      supertype, "."));
        }
        if (sub->parameters()[i].type->type().resolveOn(subContext).storageType() != superArgumentType.storageType()) {
            compatible = false;  // Boxing Thunk required for parameter i
        }
    }
    return compatible;
}

void SemanticAnalyser::finalizeProtocol(const Type &type, const Type &protocol, const SourcePosition &p) {
    for (auto method : protocol.protocol()->methodList()) {
        auto methodImplementation = type.typeDefinition()->lookupMethod(method->name(), method->isImperative());
        if (methodImplementation == nullptr) {
            package_->compiler()->error(
                    CompilerError(p, type.toString(TypeContext()), " does not conform to protocol ",
                                  protocol.toString(TypeContext()), ": Method ",
                                  utf8(method->name()), " not provided."));
            continue;
        }

        if (imported_) {
            continue;
        }

        methodImplementation->createUnspecificReification();
        auto layer = enforcePromises(methodImplementation, method, protocol, TypeContext(type), TypeContext(protocol));
        if (layer != nullptr) {
            type.typeDefinition()->addMethod(std::move(layer));
        }
    }
}

void SemanticAnalyser::checkProtocolConformance(const Type &type) {
    for (auto &protocol : type.typeDefinition()->protocols()) {
        finalizeProtocol(type, protocol->type(), protocol->position());
    }
}

void SemanticAnalyser::finalizeProtocols(const Type &type) {
    std::set<Type> protocols;

    for (auto &protocol : type.typeDefinition()->protocols()) {
        auto &protocolType = protocol->analyseType(TypeContext(type));
        if (protocolType.unboxedType() != TypeType::Protocol) {
            package_->compiler()->error(CompilerError(protocol->position(), "Type is not a protocol."));
            continue;
        }
        if (protocols.find(protocolType.unboxed()) != protocols.end()) {
            package_->compiler()->error(CompilerError(protocol->position(),
                                                      "Conformance to protocol was already declared."));
            continue;
        }
        protocols.emplace(protocolType.unboxed());
    }
}

}  // namespace EmojicodeCompiler
