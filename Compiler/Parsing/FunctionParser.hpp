//
//  FunctionParser.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef FunctionParser_hpp
#define FunctionParser_hpp

#include <utility>
#include "AST/ASTStatements.hpp"
#include "AbstractParser.hpp"
#include "Lex/TokenStream.hpp"
#include "Parsing/OperatorHelper.hpp"
#include "Types/TypeContext.hpp"

namespace EmojicodeCompiler {

class ASTNode;
class ASTBinaryOperator;

class FunctionParser : protected AbstractParser {
public:
    FunctionParser(Package *pkg, TokenStream &stream, TypeContext context)
            : AbstractParser(pkg, stream), typeContext_(std::move(context)) {}
    std::unique_ptr<ASTBlock> parse();
private:
    TypeContext typeContext_;

    std::unique_ptr<ASTStatement> parseStatement();
    ASTBlock parseBlock();

    void parseMainArguments(ASTArguments *arguments, const SourcePosition &position);
    std::shared_ptr<ASTExpr> parseExprLeft(const Token &token, int precedence);
    std::shared_ptr<ASTExpr> parseRight(std::shared_ptr<ASTExpr> left, int precendence);
    std::shared_ptr<ASTExpr> parseClosure(const Token &token);
    std::shared_ptr<ASTExpr> parseExpr(int precedence) {
        return parseExprTokens(stream_.consumeToken(), precedence);
    }

    std::unique_ptr<ASTStatement> parseIf(const SourcePosition &position);
    std::unique_ptr<ASTStatement> parseErrorHandler(const SourcePosition &position);

    std::shared_ptr<ASTExpr> parseExprTokens(const Token &token, int precendence);
    std::shared_ptr<ASTExpr> parseExprIdentifier(const Token &token);
    std::shared_ptr<ASTExpr> parseInitialization(const SourcePosition &position);

    std::shared_ptr<ASTExpr> parseCondition();
    std::shared_ptr<ASTExpr> parseGroup();

    std::shared_ptr<ASTExpr> parseTypeAsValue(const Token &token);

    ASTArguments parseArguments(const SourcePosition &position);
    std::shared_ptr<ASTTypeExpr> parseTypeExpr(const SourcePosition &p);

    template <typename T>
    std::shared_ptr<T> parseUnaryPrefix(const Token &token) {
        return std::make_shared<T>(parseExpr(kPrefixPrecedence), token.position());
    }

    template <typename T>
    std::shared_ptr<T> parseListingLiteral(Emojis end, const Token &token) {
        auto node = std::make_shared<T>(token.position());
        while (stream_.nextTokenIsEverythingBut(end)) {
            node->addValue(parseExpr(0));
        }
        stream_.consumeToken();
        return node;
    }

    int peakOperatorPrecedence();

    ASTBlock parseBlockToEnd(const SourcePosition &pos);

    /// Tries to recover from a syntax error by consuming all tokens up to a synchronization token. Must only be used
    /// at statement level.
    void recover();

    std::unique_ptr<ASTStatement> handleStatementToken(const Token &token);

    std::unique_ptr<ASTStatement> parseExprStatement(const Token &token);

    std::unique_ptr<ASTStatement> parseVariableDeclaration(const Token &token);

    std::unique_ptr<ASTStatement> parseReturn(const Token &token);
    std::unique_ptr<ASTStatement> parseAssignment(const std::shared_ptr<ASTExpr> &expr) const;
};

}  // namespace EmojicodeCompiler

#endif /* FunctionParser_hpp */
