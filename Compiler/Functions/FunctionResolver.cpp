//
//  FunctionResolver.cpp
//  emojicodec
//
//  Created by Theo Weidmann on 22.12.19.
//

#include "FunctionResolver.hpp"
#include "Initializer.hpp"
#include "Types/TypeContext.hpp"
#include "Types/Class.hpp"
#include "Analysis/GenericInferer.hpp"
#include "Compiler.hpp"
#include "Analysis/ExpressionAnalyser.hpp"
#include "Analysis/SemanticAnalyser.hpp"
#include "AST/ASTExpr.hpp"
#include <algorithm>
#include <set>

namespace EmojicodeCompiler {

bool operator <(const FunctionTableKey& x, const FunctionTableKey& y) {
    return std::tie(x.paramCount, x.name, x.mood) < std::tie(y.paramCount, y.name, y.mood);
}

template <typename T>
void FunctionResolver<T>::duplicateDeclarationCheck(T *function) {
    auto prev = map_.find(FunctionTableKey(function->name(), function->mood(), function->parameters().size()));
    if (prev != map_.end()) {
        for (auto &fn : prev->second) {
            bool duplicate = true;
            for (size_t i = 0; i < function->parameters().size(); i++) {
                if (!function->parameters()[i].type->type().identicalTo(fn->parameters()[i].type->type(), TypeContext(), nullptr)) {
                    duplicate = false;
                }
            }
            if (duplicate) {
                auto ce = CompilerError(function->position(), utf8(function->name()), " overload is declared twice.");
                ce.addNotes(fn->position(), "Previous declaration is here");
                throw ce;
            }
        }
    }
}

template <typename T>
T* FunctionResolver<T>::add(std::unique_ptr<T> &&fn) {
    auto rawPtr = fn.get();
    map_[FunctionTableKey(rawPtr->name(), rawPtr->mood(), rawPtr->parameters().size())].emplace_back(std::move(fn));
    list_.push_back(rawPtr);
    return rawPtr;
}

std::vector<Type> analyseArgs(ExpressionAnalyser *analyser, ASTArguments *args) {
    std::vector<Type> types;
    types.reserve(args->args().size());
    for (auto &arg : args->args()) {
        types.emplace_back(analyser->analyse(arg));
    }
    return types;
}

template <typename T>
struct Candidate {
    Candidate(T* function, GenericInferer genericInferer)
            : function(function), genericInferer(std::move(genericInferer)) {}
    T* function;
    GenericInferer genericInferer;
};

template <typename T>
bool FunctionResolution<T>::checkFunctionAccess(Function *function) const {
    auto callee = typeContext_.calleeType();
    if (callee.type() == TypeType::TypeAsValue) {
        callee = callee.typeOfTypeValue();
    }
    if (function->accessLevel() == AccessLevel::Private) {
        if (callee.type() != function->owner()->type().type() || function->owner() != callee.typeDefinition()) {
            // TODO: add report analyser->compiler()->error(CompilerError(p, utf8(function->name()), " is 🔒."));
            return false;
        }
    }
    else if (function->accessLevel() == AccessLevel::Protected) {
        if (callee.type() != function->owner()->type().type()
            || !callee.klass()->inheritsFrom(dynamic_cast<Class *>(function->owner()))) {
           // analyser->compiler()->error(CompilerError(p, utf8(function->name()), " is 🔐."));
           return false;
        }
    }
    return true;
}

template <typename T>
std::optional<GenericInferer> FunctionResolution<T>::checkCallSignature(Function *function) const {
    bool inferLocalArgs = genericArgs_.empty() && !function->genericParameters().empty();
    bool inferTypeArgs = callee_.genericArguments().empty() && !callee_.typeDefinition()->genericParameters().empty();

    GenericInferer inf(inferLocalArgs ? function->genericParameters().size() : 0,
                       inferTypeArgs ? callee_.typeDefinition()->genericParameters().size() : 0, analyser_);

    TypeContext callTypeContext = TypeContext(inferTypeArgs ? Type::noReturn() : callee_, function,
                                              inferLocalArgs ? nullptr : &genericArgs_);
    size_t i = 0;
    for (auto &param : function->parameters()) {
        if (!args_[i++].compatibleTo(param.type->type().resolveOn(callTypeContext), typeContext_, &inf)) {
            return std::nullopt;
        }
    }
    return inf;
}

template <typename T>
void FunctionResolution<T>::addResolver(const FunctionResolver<T> *res) {
    std::set<Function *> blocked;
    for (; res != nullptr; res = res->super_) {
        auto pos = res->map_.find(key_);
        if (pos != res->map_.end()) {
            for (auto &fn : pos->second) {
                auto pair = checkCallSignature(fn.get());
                if (pair.has_value() && checkFunctionAccess(fn.get())) {
                    if (blocked.find(fn.get()) == blocked.end()) {
                        candidates_.emplace_back(fn.get(), *pair);
                    }
                    if (fn->overriding()) {
                        blocked.emplace(fn->superFunction());
                    }
                }
            }
        }
    }
}

template <typename T>
std::optional<Candidate<T>> FunctionResolution<T>::resolve() {
    if (candidates_.empty()) { return std::nullopt; }
    if (candidates_.size() == 1) { return candidates_.front(); }

    auto error = CompilerError(SourcePosition(), "Ambiguous call to ", utf8(key_.name), ".");
    for (auto &candidate : candidates_) {
        error.addNotes(candidate.function->position(), "Found this candidate.");
    }
    throw error;
}

template <typename T>
T* FunctionResolution<T>::resolveAndReificate(ASTArguments *args, Type *type) {
    auto candidate = resolve();
    if (!candidate.has_value()) {
        return nullptr;
    }
    if (candidate->genericInferer.inferringLocal()) {
        args->genericArguments() = candidate->genericInferer.localArguments(args->position(), analyser_->compiler());
    }
    if (candidate->genericInferer.inferringType()) {
        type->setGenericArguments(candidate->genericInferer.typeArguments(args->position(), analyser_->compiler()));
        type->typeDefinition()->requestReificationAndCheck(typeContext_, type->genericArguments(), args->position());
        *type = type->resolveOnSuperArgumentsAndConstraints(typeContext_);
    }
    return candidate->function;
}

template<typename T>
FunctionResolution<T>::FunctionResolution(const std::u32string &name, Mood mood, ASTArguments *args, const Type &callee,
                                          ExpressionAnalyser *analyser)
        : key_(name, mood, args->args().size()), callee_(callee), args_(analyseArgs(analyser, args)),
          genericArgs_(transformTypeAstVector(args->genericArguments(), analyser->typeContext())),
          typeContext_(analyser->typeContext()), analyser_(analyser->semanticAnalyser()) {}

template <typename T>
FunctionResolution<T>::~FunctionResolution() = default;

template <typename T>
T* FunctionResolver<T>::lookup(const Function *prototype, const TypeContext &prototypeTc, SemanticAnalyser *analyser) const {
    std::vector<Type> signature;
    signature.reserve(prototype->parameters().size());
    for (auto &param : prototype->parameters()) {
        signature.emplace_back(param.type->type().resolveOn(prototypeTc));
    }
    return lookup(prototype->name(), prototype->mood(), signature, prototype->owner()->type(), prototypeTc, analyser);
}

template <typename T>
T* FunctionResolver<T>::lookup(const std::u32string &name, Mood mood, const std::vector<Type> &args,
                               const Type &callee, const TypeContext &typeContext, SemanticAnalyser *analyser) const {
    auto resolver = FunctionResolution<T>(name, mood, args, {}, callee, typeContext, analyser);
    resolver.addResolver(this);
    if (auto candidate = resolver.resolve()) {
        return candidate->function;
    }
    return nullptr;
}

template <typename T>
T* FunctionResolver<T>::get(const std::u32string &name, Mood mood, ASTArguments *args,
                            Type *callee, ExpressionAnalyser *analyser, const SourcePosition &p) const {
    auto resolver = FunctionResolution<T>(name, mood, args, *callee, analyser);
    resolver.addResolver(this);
    if (auto candidate = resolver.resolveAndReificate(args, callee)) {
        return candidate;
    }
    throw CompilerError(p, callee->toString(analyser->typeContext()), " has no suitable ", utf8(name), ".");
}

template class FunctionResolver<Function>;
template class FunctionResolver<Initializer>;
template class FunctionResolution<Function>;
template class FunctionResolution<Initializer>;

}  // namespace EmojicodeCompiler
