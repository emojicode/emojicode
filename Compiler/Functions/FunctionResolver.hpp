//
//  FunctionResolver.hpp
//  emojicodec
//
//  Created by Theo Weidmann on 22.12.19.
//

#ifndef FunctionResolver_hpp
#define FunctionResolver_hpp

#include <string>
#include <utility>
#include <vector>
#include <map>
#include <list>
#include <optional>
#include "AST/ASTExpr.hpp"

namespace EmojicodeCompiler {

struct SourcePosition;
class Type;
class TypeContext;
class Function;
enum class Mood;
class SemanticAnalyser;
class CompilerError;

template <typename T>
struct Candidate;
struct NonCandidate;

struct FunctionTableKey {
    FunctionTableKey(std::u32string name, Mood mood, int paramCount)
        : name(std::move(name)), mood(mood), paramCount(paramCount) {}
    std::u32string name;
    Mood mood;
    int paramCount;
};

template <typename T>
class FunctionResolution;

template <typename T>
class FunctionResolver {
    friend FunctionResolution<T>;
public:
    /// Resolves the function. If no suitable function is found, a (detailed) CompilerError is raised.
    /// Infers generic arguments for type and function if non are provided but arguments are required.
    /// The inferred arguments are stored into `type` and `args`.
    /// @note This method should always be the preferred way of method resolution.
    T* get(const std::u32string &name, Mood mood, ASTArguments *args,
           Type *callee, ExpressionAnalyser *analyser, const SourcePosition &p) const;

    /// @param typeContext The context in which the function call occurs. This is the context that is normally provided
    ///                    by an ExpressionAnalyser.
    /// Always infers function generic arguments and discards them.
    T* lookup(const std::u32string &name, Mood mood, const std::vector<Type> &args,
              const Type &callee, const TypeContext &typeContext, SemanticAnalyser *analyser) const;

    /// Tries to resolve a function that matches the prototype of the provided function.
    /// Always infers function generic arguments and discards them.
    T* lookup(const Function *prototype, const TypeContext &prototypeTc, SemanticAnalyser *analyser) const;

    T* add(std::unique_ptr<T> &&fn);

    const std::vector<T*>& list() const { return list_; }

    /// Links this resolver to another resolver which will be searched too during resolution.
    void setSuper(FunctionResolver *super) { super_ = super; }

    /// Issues an error if an identical twin of this function was declared.
    void duplicateDeclarationCheck(T *function);

private:
    std::vector<T*> list_;
    std::map<FunctionTableKey, std::vector<std::unique_ptr<T>>> map_;

    FunctionResolver *super_ = nullptr;
};

template <typename T>
class FunctionResolution {
public:
    FunctionResolution(const std::u32string &name, Mood mood, const std::vector<Type> &args,
                       std::vector<Type> genericArgs, const Type &callee,
                       const TypeContext &typeContext, SemanticAnalyser *analyser)
            : key_(name, mood, args.size()), callee_(callee), args_(args), genericArgs_(std::move(genericArgs)),
              typeContext_(typeContext), analyser_(analyser) {}

    FunctionResolution(const std::u32string &name, Mood mood, ASTArguments *args, const Type &callee,
                       ExpressionAnalyser *analyser);

    /// Adds the suitable functions of a resolver to the resolution.
    void addResolver(const FunctionResolver<T> *res);

    /// Selects the most suitable function from the resolvers.
    std::optional<Candidate<T>> resolve();

    /// Selects the most suitable function like resolve(), reificates them and sets the generic arguments in `args` and
    /// `type`.
    T* resolveAndReificate(ASTArguments *args, Type *type);

    /// Adds notes to a CompilerError explaining why no suitable candidate was found.
    /// If there are no methods with matching name, mood and parameter count, no notes will be added.
    /// @pre resolve() or resolveAndReificate() must have returned nullopt or nullptr respectively.
    void explain(CompilerError *error) const;

    ~FunctionResolution();

private:
    bool checkFunctionAccess(Function *function);
    std::optional<GenericInferer> checkCallSignature(Function *function);
    bool checkGenericArguments(Function *function, const std::vector<Type> &args);
    bool moreSpecific(Function *a, Function *b) const;

    FunctionTableKey key_;
    const Type &callee_;
    std::vector<Type> args_;
    std::vector<Type> genericArgs_;
    const TypeContext &typeContext_;
    SemanticAnalyser *analyser_;

    std::optional<Candidate<T>> pick();

    std::vector<Candidate<T>> candidates_;
    std::vector<NonCandidate> nonCandidates_;
    std::map<std::pair<Function*, Function*>, bool> moreSpecific_;
};

}  // namespace EmojicodeCompiler


#endif /* FunctionResolver_hpp */
