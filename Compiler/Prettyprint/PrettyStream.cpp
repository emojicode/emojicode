//
//  PrettyStream.cpp
//  EmojicodeCompiler
//
//  Copyright © 2018 Theo Weidmann. All rights reserved.
//

#include "PrettyStream.hpp"
#include "Lex/SourceManager.hpp"
#include "Utils/StringUtils.hpp"
#include "Types/Type.hpp"
#include "AST/ASTType.hpp"
#include "PrettyPrinter.hpp"

namespace EmojicodeCompiler {

void PrettyStream::printComments(const SourcePosition &p) {
    if (p.file == nullptr) {
        return;
    }
    p.file->findComments(lastCommentQuery_, p, [this, &p](const Token &comment) {
        if (whitespaceOffer_ == '\n') {
            if (comment.position().line >= p.line) {
                stream_ << whitespaceOffer_;
                whitespaceOffer_ = 0;
                indent();
            }
            else {
                stream_ << "  ";
            }
        }

        thisStream() << (comment.type() == TokenType::MultilineComment ? "💭🔜" : "💭") << comment.value();
        if (comment.type() == TokenType::MultilineComment) stream_ << "🔚💭";
        else offerNewLine();
    });
    lastCommentQuery_ = p;
}

void PrettyStream::setOutPath(const std::string &path) {
    stream_ = std::fstream(path, std::ios_base::out);
}

void PrettyStream::printClosure(Function *function) {
    prettyPrinter_->printClosure(function);
}

PrettyStream& PrettyStream::operator<<(ASTType *node) {
    node->toCode(*this);
    return *this;
}

PrettyStream& PrettyStream::operator<<(const std::u32string &str) {
    stream_ << utf8(str);
    return *this;
}

PrettyStream& PrettyStream::operator<<(const Type &type) {
    stream_ << type.toString(typeContext_, prettyPrinter_->package());
    return *this;
}

void PrettyStream::setLastCommentQueryPlace(const SourcePosition &p) {
    lastCommentQuery_ = p;
}

void PrettyStream::withTypeContext(const TypeContext &context, std::function<void ()> fn) {
    typeContext_ = context;
    fn();
    typeContext_ = TypeContext();
}

}  // namespace EmojicodeCompiler
