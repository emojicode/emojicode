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

namespace EmojicodeCompiler {

class ASTNode;
class ASTBinaryOperator;

class FunctionParser : protected AbstractParser {
public:
    FunctionParser(Package *pkg, TokenStream &stream) : AbstractParser(pkg, stream) {}
    std::unique_ptr<ASTBlock> parse();
    std::shared_ptr<ASTExpr> parseExpr(int precedence) {
        return parseExprTokens(stream_.consumeToken(), precedence);
    }
private:
    std::unique_ptr<ASTStatement> parseStatement();
    ASTBlock parseBlock();

    void parseMainArguments(ASTArguments *arguments, const SourcePosition &position);
    std::shared_ptr<ASTExpr> parseExprLeft(const Token &token, int precedence);
    std::shared_ptr<ASTExpr> parseRight(std::shared_ptr<ASTExpr> left, int precendence);
    std::shared_ptr<ASTExpr> parseClosure(const Token &token);

    std::unique_ptr<ASTStatement> parseIf(const SourcePosition &position);
    std::unique_ptr<ASTStatement> parseErrorHandler(const SourcePosition &position);

    std::shared_ptr<ASTExpr> parseExprTokens(const Token &token, int precendence);
    std::shared_ptr<ASTExpr> parseExprIdentifier(const Token &token);
    std::shared_ptr<ASTExpr> parseInitialization(const SourcePosition &position);
    std::shared_ptr<ASTExpr> parseInterpolation(const Token &token);

    std::shared_ptr<ASTExpr> parseCondition();
    std::shared_ptr<ASTExpr> parseGroup();

    std::shared_ptr<ASTExpr> parseTypeAsValue(const Token &token);

    std::pair<std::shared_ptr<ASTExpr>, ASTArguments> parseCalleeAndArguments(const SourcePosition &position);

    ASTArguments parseArgumentsWithoutCallee(const SourcePosition &position);
    std::shared_ptr<ASTTypeExpr> parseTypeExpr(const SourcePosition &p);

    template <typename T>
    std::shared_ptr<T> parseUnaryPrefix(const Token &token) {
        return std::make_shared<T>(parseExpr(kPrefixPrecedence), token.position());
    }

    std::shared_ptr<ASTExpr> parseListingLiteral(const SourcePosition &position);

    int peakOperatorPrecedence();

    ASTBlock parseBlockToEnd(const SourcePosition &pos);

    /// Tries to recover from a syntax error by consuming all tokens up to a synchronization token. Must only be used
    /// at statement level.
    void recover();

    std::unique_ptr<ASTStatement> handleStatementToken(const Token &token);

    std::unique_ptr<ASTStatement> parseExprStatement(const Token &token);

    std::unique_ptr<ASTStatement> parseVariableDeclaration(const Token &token);

    std::unique_ptr<ASTStatement> parseReturn(const Token &token);
    std::unique_ptr<ASTStatement> parseAssignment(std::shared_ptr<ASTExpr> expr);

    std::unique_ptr<ASTStatement> parseMethodAssignment(std::shared_ptr<ASTExpr> expr);
};

}  // namespace EmojicodeCompiler

#endif /* FunctionParser_hpp */
