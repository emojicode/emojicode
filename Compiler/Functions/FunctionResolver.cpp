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
                throw std::move(ce);
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

struct NonCandidate {
    enum class Reason {
        Argument, GenericArgument, Access
    };
    NonCandidate(Function* function, Reason reason) : function(function), reason(reason) {}
    NonCandidate(Function* function, Reason reason, size_t firstOffender)
        : function(function), reason(reason), firstOffender(firstOffender) {}
    Function* function;
    Reason reason;
    size_t firstOffender;
};

template <typename T>
bool FunctionResolution<T>::checkFunctionAccess(Function *function) {
    auto caller = typeContext_.calleeType();
    if (caller.type() == TypeType::TypeAsValue) {
        caller = caller.typeOfTypeValue();
    }
    if ((function->accessLevel() == AccessLevel::Private &&
         (caller.type() != callee_.type() || callee_.typeDefinition() != caller.typeDefinition())) ||
        (function->accessLevel() == AccessLevel::Protected &&
         (caller.type() != callee_.type() || !caller.klass()->inheritsFrom(callee_.klass())))) {
        nonCandidates_.emplace_back(function, NonCandidate::Reason::Access);
        return false;
    }
    return true;
}

template <typename T>
std::optional<GenericInferer> FunctionResolution<T>::checkCallSignature(Function *function) {
    bool inferLocalArgs = genericArgs_.empty() && !function->genericParameters().empty();
    bool inferTypeArgs = callee_.genericArguments().empty() && !callee_.typeDefinition()->genericParameters().empty();

    GenericInferer inf(inferLocalArgs ? function->genericParameters().size() : 0,
                       inferTypeArgs ? callee_.typeDefinition()->genericParameters().size() : 0, analyser_);

    TypeContext callTypeContext = TypeContext(inferTypeArgs ? Type::noReturn() : callee_, function,
                                              inferLocalArgs ? nullptr : &genericArgs_);
    size_t i = 0;
    for (auto &param : function->parameters()) {
        if (!args_[i++].compatibleTo(param.type->type().resolveOn(callTypeContext), typeContext_, &inf)) {
            nonCandidates_.emplace_back(function, NonCandidate::Reason::Argument, i);
            return std::nullopt;
        }
    }
    return inf;
}

template <typename T>
bool FunctionResolution<T>::checkGenericArguments(Function *function, const std::vector<Type> &args) {
    for (size_t i = function->offset(); i < args.size(); i++) {
        if (!args[i].compatibleTo(function->constraintForIndex(i), TypeContext(callee_, function))) {
            nonCandidates_.emplace_back(function, NonCandidate::Reason::GenericArgument, i);
            return false;
        }
    }
    return true;
}

template <typename T>
void FunctionResolution<T>::addResolver(const FunctionResolver<T> *res) {
    std::set<Function *> blocked;
    for (; res != nullptr; res = res->super_) {
        auto pos = res->map_.find(key_);
        if (pos != res->map_.end()) {
            for (auto &fn : pos->second) {
                if (blocked.find(fn.get()) != blocked.end()) {
                    if (fn->overriding()) {
                        blocked.emplace(fn->superFunction());
                    }
                    continue;
                }

                auto inf = checkCallSignature(fn.get());
                if (inf.has_value() &&
                    checkFunctionAccess(fn.get()) &&
                    checkGenericArguments(fn.get(), inf->localArgumentsType(SourcePosition(), analyser_->compiler()))) {
                    candidates_.emplace_back(fn.get(), *inf);
                    if (fn->overriding()) {
                        blocked.emplace(fn->superFunction());
                    }
                }
            }
        }
    }
}

template <typename T>
bool FunctionResolution<T>::moreSpecific(Function *a, Function *b) const {
    int score = 0;  // one point for each parameter where a is more specific
    for (size_t i = 0; i < a->parameters().size(); i++) {
        auto at = a->parameters()[i].type->type();
        auto bt = b->parameters()[i].type->type();
        if (at.resolveOnSuperArgumentsAndConstraints(TypeContext(callee_, a))
                .compatibleTo(bt, TypeContext(callee_, b), nullptr)) {
            score++;
        }
    }
    return score > a->parameters().size() / 2;
}

template <typename T>
std::optional<Candidate<T>> FunctionResolution<T>::resolve() {
    if (candidates_.empty()) { return std::nullopt; }
    if (auto f = pick()) { return f; }

    std::sort(candidates_.begin(), candidates_.end(), [&](auto &a, auto &b) {
        auto ms = moreSpecific(a.function, b.function);
        moreSpecific_.emplace(std::make_pair(a.function, b.function), ms);
        return ms;
    });

    size_t i = 1;
    for (; i < candidates_.size(); i++) {
        auto a = candidates_[i - 1].function, b = candidates_[i].function;
        auto ms = moreSpecific_.find(std::make_pair(a, b));
        if ((ms != moreSpecific_.end() && ms->second) || moreSpecific(a, b)) {
            break;
        }
    }
    candidates_.erase(candidates_.begin() + i, candidates_.end());

    if (auto f = pick()) { return f; }

    auto error = CompilerError(SourcePosition(), "Ambiguous call to ", utf8(key_.name), ".");
    for (auto &candidate : candidates_) {
        error.addNotes(candidate.function->position(), "Found this candidate.");
    }
    throw error;
}

template <typename T>
std::optional<Candidate<T>> FunctionResolution<T>::pick() {
    if (candidates_.size() == 1) { return candidates_.front(); }

    Candidate<T> *picked = nullptr;
    for (auto &candidate : candidates_) {
        if (candidate.function->genericParameters().empty()) {
            if (picked != nullptr) {  // more than one non-generic function
                return std::nullopt;
            }
            picked = &candidate;
        }
    }
    if (picked == nullptr) {
        return std::nullopt;
    }
    return *picked;
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

template <typename T>
void FunctionResolution<T>::explain(CompilerError *error) const {
    for (auto &nc : nonCandidates_) {
        switch (nc.reason) {
        case NonCandidate::Reason::Access:
            error->addNotes(nc.function->position(), "This candidate is not accessible.");
        case NonCandidate::Reason::Argument:
            error->addNotes(nc.function->position(), "Argument number ", nc.firstOffender,
                            " is not compatible to the expected type.");
        case NonCandidate::Reason::GenericArgument:
            error->addNotes(nc.function->position(), "Generic argument number ", nc.firstOffender,
                            " is not compatible to the expected type.");
        }
    }
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
    auto error = CompilerError(p, callee->toString(analyser->typeContext()), " has no suitable ", utf8(name), ".");
    resolver.explain(&error);
    throw std::move(error);
}

template class FunctionResolver<Function>;
template class FunctionResolver<Initializer>;
template class FunctionResolution<Function>;
template class FunctionResolution<Initializer>;

}  // namespace EmojicodeCompiler
