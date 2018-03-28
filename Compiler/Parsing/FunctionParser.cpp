//
//  FunctionParser.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Lex/Token.hpp"
#include "AST/ASTBinaryOperator.hpp"
#include "AST/ASTClosure.hpp"
#include "AST/ASTControlFlow.hpp"
#include "AST/ASTInitialization.hpp"
#include "AST/ASTLiterals.hpp"
#include "AST/ASTMethod.hpp"
#include "AST/ASTNode.hpp"
#include "AST/ASTTypeExpr.hpp"
#include "AST/ASTUnary.hpp"
#include "AST/ASTVariables.hpp"
#include "AST/ASTUnsafeBlock.hpp"
#include "Package/Package.hpp"
#include "Compiler.hpp"
#include "FunctionParser.hpp"

namespace EmojicodeCompiler {

std::shared_ptr<ASTBlock> FunctionParser::parse() {
    return std::make_shared<ASTBlock>(parseBlockToEnd(SourcePosition(0, 0, "")));
}

ASTBlock FunctionParser::parseBlock() {
    auto token = stream_.consumeToken(TokenType::BlockBegin);
    return parseBlockToEnd(token.position());
}

ASTBlock FunctionParser::parseBlockToEnd(const SourcePosition &pos) {
    auto block = ASTBlock(SourcePosition(0, 0, ""));
    while (stream_.nextTokenIsEverythingBut(TokenType::BlockEnd)) {
        block.appendNode(parseStatement());
    }
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

    while (stream_.consumeTokenIf(E_SPIRAL_SHELL)) {
        args.addGenericArgument(parseType(typeContext_));
    }

    parseMainArguments(&args, position);
    return args;
}

void FunctionParser::parseMainArguments(ASTArguments *arguments, const SourcePosition &position) {
    while (stream_.nextTokenIsEverythingBut(TokenType::EndArgumentList) &&
           stream_.nextTokenIsEverythingBut(TokenType::EndInterrogativeArgumentList)) {
        arguments->addArguments(parseExpr(0));
    }
    arguments->setImperative(stream_.consumeToken().type() == TokenType::EndArgumentList);
}

std::shared_ptr<ASTStatement> FunctionParser::parseStatement() {
    const Token token = stream_.consumeToken();
    try {
        return handleStatementToken(token);
    }
    catch (CompilerError &e) {
        package_->compiler()->error(e);
        recover();
    }
    return std::make_shared<ASTBlock>(token.position());
}

std::shared_ptr<ASTStatement> FunctionParser::handleStatementToken(const Token &token) {
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
            return std::make_shared<ASTRepeatWhile>(cond, block, token.position());
        }
        case TokenType::Unsafe:
            return std::make_shared<ASTUnsafeBlock>(parseBlock(), token.position());
        case TokenType::ForIn: {
            auto variableToken = stream_.consumeToken(TokenType::Variable);
            auto iteratee = parseExpr(0);
            auto block = parseBlock();
            return std::make_shared<ASTForIn>(iteratee, variableToken.value(), block, token.position());
        }
        case TokenType::Error:
            return std::make_shared<ASTRaise>(parseExpr(0), token.position());
        case TokenType::Return:
            return parseReturn(token);
        default:
            // None of the TokenTypes that begin a statement were detected so this must be an expression
            return parseExprStatement(token);
    }
}

std::shared_ptr<ASTStatement> FunctionParser::parseReturn(const Token &token) {
    auto value = stream_.consumeTokenIf(TokenType::Return) ? nullptr : parseExpr(0);
    return std::make_shared<ASTReturn>(value, token.position());
}

std::shared_ptr<ASTStatement> FunctionParser::parseVariableDeclaration(const Token &token) {
    stream_.consumeToken(TokenType::New);
    auto varName = stream_.consumeToken(TokenType::Variable);

    Type type = parseType(typeContext_);
    return std::make_shared<ASTVariableDeclaration>(type, varName.value(), token.position());
}

std::shared_ptr<ASTStatement> FunctionParser::parseExprStatement(const Token &token) {
    if (token.type() == TokenType::Variable && stream_.nextTokenIs(TokenType::LeftProductionOperator)) {
        stream_.consumeToken();
        auto opType = operatorType(stream_.consumeToken(TokenType::Operator).value());
        auto get = std::make_shared<ASTGetVariable>(token.value(), token.position());
        auto opExpr = std::make_shared<ASTBinaryOperator>(opType, get, parseExpr(0), token.position());
        return std::make_shared<ASTVariableAssignment>(token.value(), opExpr, token.position());
    }

    auto expr = parseExprTokens(token, 0);
    if (stream_.nextTokenIs(TokenType::RightProductionOperator)) {
        return parseAssignment(expr);

    }
    return std::make_shared<ASTExprStatement>(expr, token.position());
}

std::shared_ptr<ASTStatement> FunctionParser::parseAssignment(const std::shared_ptr<ASTExpr> &expr) const {
    auto rightToken = stream_.consumeToken();
    if (stream_.consumeTokenIf(TokenType::Mutable)) {
        if (stream_.consumeTokenIf(TokenType::New)) {
            auto varName = stream_.consumeToken(TokenType::Variable);
            return std::make_shared<ASTVariableDeclareAndAssign>(varName.value(), expr, rightToken.position());
        }

        auto varName = stream_.consumeToken(TokenType::Variable);
        return std::make_shared<ASTVariableAssignment>(varName.value(), expr, rightToken.position());
    }

    auto varName = stream_.consumeToken(TokenType::Variable);
    return std::make_shared<ASTConstantVariable>(varName.value(), expr, rightToken.position());
}

std::shared_ptr<ASTStatement> FunctionParser::parseErrorHandler(const SourcePosition &position) {
    auto variableToken = stream_.consumeToken(TokenType::Variable);
    auto value = parseExpr(0);
    auto valueBlock = parseBlock();

    stream_.consumeToken(TokenType::Else);
    auto errorVariableToken = stream_.consumeToken(TokenType::Variable);
    auto errorBlock = parseBlock();
    return std::make_shared<ASTErrorHandler>(value, variableToken.value(), errorVariableToken.value(),
                                             valueBlock, errorBlock, position);
}

std::shared_ptr<ASTStatement> FunctionParser::parseIf(const SourcePosition &position) {
    auto node = std::make_shared<ASTIf>(position);
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
        OperatorType type = operatorType(token.value());
        if (type == OperatorType::CallOperator) {
            left = std::make_shared<ASTCallableCall>(left, parseArguments(token.position()), token.position());
        }
        else {
            auto right = parseExpr(peakedPre);
            left = std::make_shared<ASTBinaryOperator>(type, left, right, token.position());
        }
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
            return std::make_shared<ASTSymbolLiteral>(token.value().front(), token.position());
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
    Type t = parseType(typeContext_);
    validateTypeAsValueType(token, t, typeContext_);
    return std::make_shared<ASTTypeAsValue>(t, token.position());
}

std::shared_ptr<ASTExpr> FunctionParser::parseExprIdentifier(const Token &token) {
    switch (token.value()[0]) {
        case E_TRAFFIC_LIGHT:
            return parseUnaryPrefix<ASTIsError>(token);
        case E_WHITE_LARGE_SQUARE:
            return parseUnaryPrefix<ASTMetaTypeFromInstance>(token);
        case E_BEER_MUG:
            return parseUnaryPrefix<ASTUnwrap>(token);
        case E_SCALES: {
            Type t = parseType(typeContext_);
            return std::make_shared<ASTSizeOf>(t, token.position());
        }
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
    auto name = stream_.consumeToken().value();  // TODO: TokenType::Identifier
    return std::make_shared<ASTInitialization>(name, type, parseArguments(position), position);
}

std::shared_ptr<ASTExpr> FunctionParser::parseClosure(const Token &token) {
    auto function = std::make_unique<Function>(std::u32string(1, E_GRAPES), AccessLevel::Public, true, Type::noReturn(),
                                               package_, token.position(), false, std::u32string(), false, false, true,
                                               false, FunctionType::Closure);

    parseParameters(function.get(), typeContext_);
    parseReturnType(function.get(), typeContext_);

    function->setAst(factorFunctionParser(package_, stream_, typeContext_, function.get())->parse());
    return std::make_shared<ASTClosure>(std::move(function), token.position());
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
    Type ot = parseType(typeContext_);
    switch (ot.type()) {
        case TypeType::GenericVariable:
            throw CompilerError(p, "Generic Arguments are not yet available for reflection.");
        case TypeType::Class:
            return std::make_shared<ASTStaticType>(ot, p);
        case TypeType::LocalGenericVariable:
            throw CompilerError(p, "Function Generic Arguments are not available for reflection.");
        default:
            break;
    }
    return std::make_shared<ASTStaticType>(ot, p);
}

}  // namespace EmojicodeCompiler
