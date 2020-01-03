//
//  GenericInferer.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 03.08.19.
//

#ifndef GenericInferer_h
#define GenericInferer_h

#include "Types/CommonTypeFinder.hpp"

namespace EmojicodeCompiler {

class GenericInferer {  // TODO: store TypeContext in attribute
public:
    GenericInferer(size_t localCount, size_t typeCount, SemanticAnalyser *analyser)
        : local_(localCount, CommonTypeFinder(analyser)), type_(typeCount, CommonTypeFinder(analyser)) {}
    bool inferringLocal() const { return !local_.empty(); }
    bool inferringType() const { return !type_.empty(); }

    void addLocal(size_t index, const Type &type, const TypeContext &tc) { local_[index].addType(type, tc); }
    void addType(size_t index, const Type &type, const TypeContext &tc) { type_[index].addType(type, tc); }

    std::vector<std::shared_ptr<ASTType>> localArguments(const SourcePosition &p, Compiler *compiler) const {
        std::vector<std::shared_ptr<ASTType>> result;
        for (auto &finder : local_) {
            auto commonType = finder.getCommonType(p, compiler);
            result.emplace_back(std::make_unique<ASTLiteralType>(commonType));
        }
        return result;
    }

    std::vector<Type> localArgumentsType(const SourcePosition &p, Compiler *compiler) const {
        std::vector<Type> result;
        for (auto &finder : local_) {
            result.emplace_back(finder.getCommonType(p, compiler));
        }
        return result;
    }

    std::vector<Type> typeArguments(const SourcePosition &p, Compiler *compiler) const {
        std::vector<Type> result;
        for (auto &finder : type_) {
            result.emplace_back(finder.getCommonType(p, compiler));
        }
        return result;
    }

private:
    std::vector<CommonTypeFinder> local_;
    std::vector<CommonTypeFinder> type_;
};

}  // namespace EmojicodeCompiler

#endif /* GenericInferer_h */
