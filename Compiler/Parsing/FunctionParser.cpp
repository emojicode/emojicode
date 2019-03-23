//
//  FunctionParser.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "FunctionParser.hpp"
#include "AST/ASTBinaryOperator.hpp"
#include "AST/ASTCast.hpp"
#include "AST/ASTClosure.hpp"
#include "AST/ASTControlFlow.hpp"
#include "AST/ASTInitialization.hpp"
#include "AST/ASTLiterals.hpp"
#include "AST/ASTMethod.hpp"
#include "AST/ASTNode.hpp"
#include "AST/ASTTypeExpr.hpp"
#include "AST/ASTUnary.hpp"
#include "AST/ASTUnsafeBlock.hpp"
#include "AST/ASTVariables.hpp"
#include "AST/ASTSuper.hpp"
#include "AST/ASTConditionalAssignment.hpp"
#include "AST/ASTTypeAsValue.hpp"
#include "Compiler.hpp"
#include "Lex/Token.hpp"
#include "Package/Package.hpp"

namespace EmojicodeCompiler {

std::unique_ptr<ASTBlock> FunctionParser::parse() {
    return std::make_unique<ASTBlock>(parseBlockToEnd(SourcePosition()));
}

ASTBlock FunctionParser::parseBlock() {
    auto token = stream_.consumeToken(TokenType::BlockBegin);
    return parseBlockToEnd(token.position());
}

ASTBlock FunctionParser::parseBlockToEnd(const SourcePosition &pos) {
    auto block = ASTBlock(pos);
    block.setBeginIndex(stream_.index());
    while (stream_.nextTokenIsEverythingBut(TokenType::BlockEnd)) {
        block.appendNode(parseStatement());
    }
    block.setEndIndex(stream_.index());
    stream_.consumeToken();
    return block;
}

void FunctionParser::recover() {
    size_t blockLevel = 0;
    while (stream_.nextTokenIsEverythingBut(TokenType::BlockEnd) || blockLevel-- > 0) {
        auto token = stream_.consumeToken();
        if (token.type() == TokenType::BlockBegin) {
            blockLevel++;
        }
    }
}

std::shared_ptr<ASTExpr> FunctionParser::parseCondition() {
    auto expr = parseExpr(0);
    if (stream_.consumeTokenIf(TokenType::RightProductionOperator)) {
        auto varName = stream_.consumeToken(TokenType::Variable);
        return std::make_shared<ASTConditionalAssignment>(varName.value(), expr, varName.position());
    }
    return expr;
}

ASTArguments FunctionParser::parseArguments(const SourcePosition &position) {
    auto args = ASTArguments(position);
    parseGenericArguments(&args);
    parseMainArguments(&args, position);
    return args;
}

void FunctionParser::parseMainArguments(ASTArguments *arguments, const SourcePosition &position) {
    while (stream_.nextTokenIsEverythingBut(TokenType::EndArgumentList) &&
           stream_.nextTokenIsEverythingBut(TokenType::EndInterrogativeArgumentList)) {
        arguments->addArguments(parseExpr(0));
    }
    arguments->setMood(stream_.consumeToken().type() == TokenType::EndArgumentList ? Mood::Imperative :
                                                                                     Mood::Interogative);
}

std::unique_ptr<ASTStatement> FunctionParser::parseStatement() {
    const Token token = stream_.consumeToken();
    try {
        auto stmt = handleStatementToken(token);
        if (stream_.skipsBlankLine()) {
            stmt->setParagraph();
        }
        return stmt;
    }
    catch (CompilerError &e) {
        package_->compiler()->error(e);
        recover();
    }
    return std::make_unique<ASTBlock>(token.position());
}

std::unique_ptr<ASTStatement> FunctionParser::handleStatementToken(const Token &token) {
    switch (token.type()) {
        case TokenType::Mutable:
            return parseVariableDeclaration(token);
        case TokenType::If:
            return parseIf(token.position());
        case TokenType::ErrorHandler:
            return parseErrorHandler(token.position());
        case TokenType::RepeatWhile: {
            auto cond = parseCondition();
            auto block = parseBlock();
            return std::make_unique<ASTRepeatWhile>(cond, std::move(block), token.position());
        }
        case TokenType::Unsafe:
            return std::make_unique<ASTUnsafeBlock>(parseBlock(), token.position());
        case TokenType::ForIn: {
            auto variableToken = stream_.consumeToken(TokenType::Variable);
            auto iteratee = parseExpr(0);
            auto block = parseBlock();
            return std::make_unique<ASTForIn>(iteratee, variableToken.value(), std::move(block), token.position());
        }
        case TokenType::Error:
            return std::make_unique<ASTRaise>(parseExpr(0), token.position());
        case TokenType::Return:
            return parseReturn(token);
        default:
            // None of the TokenTypes that begin a statement were detected so this must be an expression
            return parseExprStatement(token);
    }
}

std::unique_ptr<ASTStatement> FunctionParser::parseReturn(const Token &token) {
    auto value = stream_.consumeTokenIf(TokenType::Return) ? nullptr : parseExpr(0);
    return std::make_unique<ASTReturn>(value, token.position());
}

std::unique_ptr<ASTStatement> FunctionParser::parseVariableDeclaration(const Token &token) {
    stream_.consumeToken(TokenType::New);
    auto varName = stream_.consumeToken(TokenType::Variable);

    return std::make_unique<ASTVariableDeclaration>(parseType(), varName.value(), token.position());
}

std::unique_ptr<ASTStatement> FunctionParser::parseExprStatement(const Token &token) {
    if (token.type() == TokenType::Variable && stream_.nextTokenIs(TokenType::LeftProductionOperator)) {
        stream_.consumeToken();
        auto opType = operatorType(stream_.consumeToken(TokenType::Operator).value());
        return std::make_unique<ASTOperatorAssignment>(token.value(), parseExpr(0), token.position(), opType);
    }

    auto expr = parseExprTokens(token, 0);
    if (stream_.nextTokenIs(TokenType::RightProductionOperator)) {
        return parseAssignment(std::move(expr));

    }
    return std::make_unique<ASTExprStatement>(expr, token.position());
}

std::unique_ptr<ASTStatement> FunctionParser::parseAssignment(std::shared_ptr<ASTExpr> expr) {
    auto rightToken = stream_.consumeToken();
    if (stream_.consumeTokenIf(TokenType::Mutable)) {
        if (stream_.consumeTokenIf(TokenType::New)) {
            auto varName = stream_.consumeToken(TokenType::Variable);
            return std::make_unique<ASTVariableDeclareAndAssign>(varName.value(), std::move(expr),
                                                                 rightToken.position());
        }

        auto varName = stream_.consumeToken(TokenType::Variable);
        return std::make_unique<ASTVariableAssignment>(varName.value(), std::move(expr), rightToken.position());
    }
    if (stream_.nextTokenIs(TokenType::Identifier)) {
        return parseMethodAssignment(std::move(expr));
    }

    auto varName = stream_.consumeToken(TokenType::Variable);
    return std::make_unique<ASTConstantVariable>(varName.value(), expr, rightToken.position());
}

std::unique_ptr<ASTStatement> FunctionParser::parseMethodAssignment(std::shared_ptr<ASTExpr> expr) {
    auto name = stream_.consumeToken();
    auto callee = parseExpr(0);
    auto args = parseArguments(name.position());
    args.args().emplace(args.args().begin(), expr);
    if (args.mood() != Mood::Imperative) {
        package_->compiler()->error(CompilerError(args.position(), "Expected ❗️."));
    }
    args.setMood(Mood::Assignment);
    auto method = std::make_unique<ASTMethod>(name.value(), callee, args, name.position());
    return std::make_unique<ASTExprStatement>(std::move(method), name.position());
}

std::unique_ptr<ASTStatement> FunctionParser::parseErrorHandler(const SourcePosition &position) {
    auto variable = stream_.nextTokenIs(TokenType::Variable) ? stream_.consumeToken().value() : U"";
    auto value = parseExpr(0);
    auto valueBlock = parseBlock();

    stream_.consumeToken(TokenType::Else);
    auto errorVariableToken = stream_.consumeToken(TokenType::Variable);
    auto errorBlock = parseBlock();
    return std::make_unique<ASTErrorHandler>(value, variable, errorVariableToken.value(),
                                             std::move(valueBlock), std::move(errorBlock), position);
}

std::unique_ptr<ASTStatement> FunctionParser::parseIf(const SourcePosition &position) {
    auto node = std::make_unique<ASTIf>(position);
    do {
        node->addCondition(parseCondition());
        node->addBlock(parseBlock());
    } while (stream_.consumeTokenIf(TokenType::ElseIf));

    if (stream_.consumeTokenIf(TokenType::Else)) {
        node->addBlock(parseBlock());
    }
    return node;
}

int FunctionParser::peakOperatorPrecedence() {
    if (stream_.nextTokenIs(TokenType::Operator)) {
        return operatorPrecedence(operatorType(stream_.nextToken().value()));
    }
    return 0;
}

std::shared_ptr<ASTExpr> FunctionParser::parseExprTokens(const Token &token, int precendence) {
    return parseRight(parseExprLeft(token, precendence), precendence);
}

std::shared_ptr<ASTExpr> FunctionParser::parseRight(std::shared_ptr<ASTExpr> left, int precendence) {
    int peakedPre;
    while (precendence < (peakedPre = peakOperatorPrecedence())) {
        auto token = stream_.consumeToken();
        auto right = parseExpr(peakedPre);
        left = std::make_shared<ASTBinaryOperator>(operatorType(token.value()), left, right, token.position());
    }
    return left;
}

std::shared_ptr<ASTExpr> FunctionParser::parseExprLeft(const EmojicodeCompiler::Token &token, int precedence) {
    switch (token.type()) {
        case TokenType::String:
            return std::make_shared<ASTStringLiteral>(token.value(), token.position());
        case TokenType::BooleanTrue:
            return std::make_shared<ASTBooleanTrue>(token.position());
        case TokenType::BooleanFalse:
            return std::make_shared<ASTBooleanFalse>(token.position());
        case TokenType::Integer: {
            int64_t value = std::stoll(utf8(token.value()), nullptr, 0);
            return std::make_shared<ASTNumberLiteral>(value, token.value(), token.position());
        }
        case TokenType::Double: {
            double d = std::stod(utf8(token.value()));
            return std::make_shared<ASTNumberLiteral>(d, token.value(), token.position());
        }
        case TokenType::Symbol:
            throw CompilerError(token.position(), "🔣 has been removed.");
        case TokenType::Variable:
            return std::make_shared<ASTGetVariable>(token.value(), token.position());
        case TokenType::Identifier:
            return parseExprIdentifier(token);
        case TokenType::GroupBegin:
            return parseGroup();
        case TokenType::BlockBegin:
            return parseClosure(token);
        case TokenType::New:
            return parseInitialization(token.position());
        case TokenType::NoValue:
            return std::make_shared<ASTNoValue>(token.position());
        case TokenType::This:
            return std::make_shared<ASTThis>(token.position());
        case TokenType::Super: {
            auto initializerToken = stream_.consumeToken();
            auto arguments = parseArguments(token.position());
            return std::make_shared<ASTSuper>(initializerToken.value(), arguments, token.position());
        }
        case TokenType::Call: {
            auto expr = parseExpr(kPrefixPrecedence);
            return std::make_shared<ASTCallableCall>(expr, parseArguments(token.position()),
                                                     token.position());
        }
        case TokenType::Class:
        case TokenType::Enumeration:
        case TokenType::ValueType:
        case TokenType::Protocol:
            return parseTypeAsValue(token);
        default:
            throw CompilerError(token.position(), "Unexpected token ", token.stringName(), ".");
    }
}

std::shared_ptr<ASTExpr> FunctionParser::parseTypeAsValue(const Token &token) {
    return std::make_shared<ASTTypeAsValue>(parseType(), token.type(), token.position());
}

std::shared_ptr<ASTExpr> FunctionParser::parseExprIdentifier(const Token &token) {
    switch (token.value()[0]) {
        case E_BEER_MUG:
            return parseUnaryPrefix<ASTUnwrap>(token);
        case E_SCALES:
            return std::make_shared<ASTSizeOf>(parseType(), token.position());
        case E_BLACK_SQUARE_BUTTON: {
            auto expr = parseExpr(kPrefixPrecedence);
            return std::make_shared<ASTCast>(expr, parseTypeExpr(token.position()), token.position());
        }
        case E_COOKIE:
            return parseListingLiteral<ASTConcatenateLiteral>(E_COOKIE, token);
        case E_ICE_CREAM:
            return parseListingLiteral<ASTListLiteral>(E_AUBERGINE, token);
        case E_HONEY_POT:
            return parseListingLiteral<ASTDictionaryLiteral>(E_AUBERGINE, token);
        case E_IZAKAYA_LANTERN:
            return std::make_shared<ASTIsOnlyReference>(stream_.consumeToken(TokenType::Variable).value(),
                                                        token.position());
        case E_RED_TRIANGLE_POINTED_UP:
            return parseUnaryPrefix<ASTReraise>(token);
        default: {
            auto value = parseExpr(0);
            return std::make_shared<ASTMethod>(token.value(), value, parseArguments(token.position()),
                                               token.position());
        }
    }
}

std::shared_ptr<ASTExpr> FunctionParser::parseGroup() {
    auto expr = parseExpr(0);
    stream_.consumeToken(TokenType::GroupEnd);
    return expr;
}

std::shared_ptr<ASTExpr> FunctionParser::parseInitialization(const SourcePosition &position) {
    auto type = parseTypeExpr(position);
    auto name = stream_.nextTokenIs(TokenType::New) ? stream_.consumeToken().value() :
                                                      stream_.consumeToken(TokenType::Identifier).value();
    return std::make_shared<ASTInitialization>(name, type, parseArguments(position), position);
}

std::shared_ptr<ASTExpr> FunctionParser::parseClosure(const Token &token) {
    auto function = std::make_unique<Function>(std::u32string(1, E_GRAPES), AccessLevel::Public, true, nullptr,
                                               package_, token.position(), false, std::u32string(), false, false,
                                               Mood::Imperative, false, FunctionType::Function, false);

    bool escaping = stream_.consumeTokenIf(E_LEFT_LUGGAGE);

    parseParameters(function.get(), false, false);
    parseReturnType(function.get());
    parseErrorType(function.get());

    function->setAst(FunctionParser(package_, stream_).parse());
    return std::make_shared<ASTClosure>(std::move(function), token.position(), escaping);
}

std::shared_ptr<ASTTypeExpr> FunctionParser::parseTypeExpr(const SourcePosition &p) {
    if (stream_.consumeTokenIf(E_BLACK_LARGE_SQUARE)) {
        return std::make_shared<ASTTypeFromExpr>(parseExpr(kPrefixPrecedence), p);
    }
    if (stream_.consumeTokenIf(E_MEDIUM_BLACK_CIRCLE)) {
        return std::make_shared<ASTInferType>(p);
    }
    if (stream_.consumeTokenIf(TokenType::This)) {
        return std::make_shared<ASTThisType>(p);
    }
    return std::make_shared<ASTStaticType>(parseType(), p);
}

}  // namespace EmojicodeCompiler
