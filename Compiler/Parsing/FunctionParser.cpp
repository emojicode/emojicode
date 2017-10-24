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
#include "FunctionParser.hpp"

namespace EmojicodeCompiler {

std::shared_ptr<ASTBlock> FunctionParser::parse() {
    auto block = std::make_shared<ASTBlock>(SourcePosition(0, 0, ""));
    while (stream_.nextTokenIsEverythingBut(TokenType::BlockEnd)) {
        block->appendNode(parseStatement());
    }
    stream_.consumeToken();
    return block;
}

ASTBlock FunctionParser::parseBlock() {
    auto &token = stream_.consumeToken(TokenType::BlockBegin);

    auto block = ASTBlock(token.position());
    while (stream_.nextTokenIsEverythingBut(TokenType::BlockEnd)) {
        block.appendNode(parseStatement());
    }
    stream_.consumeToken();
    return block;
}

std::shared_ptr<ASTExpr> FunctionParser::parseCondition() {
    if (stream_.consumeTokenIf(TokenType::FrozenDeclaration)) {
        auto &varName = stream_.consumeToken(TokenType::Variable);
        return std::make_shared<ASTConditionalAssignment>(varName.value(), parseExpr(0), varName.position());
    }
    return parseExpr(0);
}

ASTArguments FunctionParser::parseArguments(const SourcePosition &position) {
    auto args = ASTArguments(position);

    while (stream_.consumeTokenIf(E_SPIRAL_SHELL)) {
        args.addGenericArgument(parseType(typeContext_, TypeDynamism::AllKinds));
    }

    if (stream_.consumeTokenIf(TokenType::EndArgumentList)) {
        return args;
    }

    parseMainArguments(&args, position);
    return args;
}

void FunctionParser::parseMainArguments(ASTArguments *arguments, const SourcePosition &position) {
    stream_.consumeToken(TokenType::BeginArgumentList);
    while (stream_.nextTokenIsEverythingBut(TokenType::EndArgumentList)) {
        arguments->addArguments(parseExpr(0));
    }
    stream_.consumeToken();
}

std::shared_ptr<ASTStatement> FunctionParser::parseStatement() {
    const Token &token = stream_.consumeToken();
    switch (token.type()) {
        case TokenType::Declaration: {
            auto &varName = stream_.consumeToken(TokenType::Variable);

            Type type = parseType(typeContext_, TypeDynamism::AllKinds);
            return std::make_shared<ASTVariableDeclaration>(type, varName.value(), token.position());
        }
        case TokenType::Assignment:
            return parseVariableAssignment(token);
        case TokenType::FrozenDeclaration: {
            auto &varName = stream_.consumeToken(TokenType::Variable);
            return std::make_shared<ASTFrozenDeclaration>(varName.value(), parseExpr(0), token.position());
        }
        case TokenType::If:
            return parseIf(token.position());
        case TokenType::ErrorHandler:
            return parseErrorHandler(token.position());
        case TokenType::RepeatWhile: {
            auto cond = parseCondition();
            auto block = parseBlock();
            return std::make_shared<ASTRepeatWhile>(cond, block, token.position());
        }
        case TokenType::ForIn: {
            auto &variableToken = stream_.consumeToken(TokenType::Variable);
            auto iteratee = parseExpr(0);
            auto block = parseBlock();
            return std::make_shared<ASTForIn>(iteratee, variableToken.value(), block, token.position());
        }
        case TokenType::SuperInit: {
            auto &initializerToken = stream_.consumeToken();  // TODO: TokenType::Identifier
            auto arguments = parseArguments(token.position());
            return std::make_shared<ASTSuperinitializer>(initializerToken.value(), arguments, token.position());
        }
        case TokenType::Error:
            return std::make_shared<ASTRaise>(parseExpr(0), token.position());
        case TokenType::Return:
            return std::make_shared<ASTReturn>(parseExpr(0), token.position());
        default:
            // None of the TokenTypes that begin a statement were detected so this must be an expression
            return std::make_shared<ASTExprStatement>(parseExprTokens(token, 0), token.position());
    }
}

std::shared_ptr<ASTStatement> FunctionParser::parseErrorHandler(const SourcePosition &position) {
    auto &variableToken = stream_.consumeToken(TokenType::Variable);
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

std::shared_ptr<ASTStatement> FunctionParser::parseVariableAssignment(const Token &token) {
    if (stream_.nextToken().type() == TokenType::Identifier) {
        auto &method = stream_.consumeToken();
        auto &varName = stream_.consumeToken(TokenType::Variable);

        auto varGet = std::make_shared<ASTGetVariable>(varName.value(), token.position());
        auto mc = std::make_shared<ASTMethod>(method.value(), varGet, parseArguments(token.position()),
                                              token.position());
        return std::make_shared<ASTVariableAssignmentDecl>(varName.value(), mc, token.position());
    }

    auto &varName = stream_.consumeToken(TokenType::Variable);
    if (stream_.nextToken().type() == TokenType::Operator) {
        auto &token = stream_.consumeToken();

        auto varGet = std::make_shared<ASTGetVariable>(varName.value(), token.position());
        auto mc = std::make_shared<ASTBinaryOperator>(operatorType(token.value()), varGet,
                                                      parseExpr(0), token.position());
        return std::make_shared<ASTVariableAssignmentDecl>(varName.value(), mc, token.position());
    }

    return std::make_shared<ASTVariableAssignmentDecl>(varName.value(), parseExpr(0), token.position());
}

int FunctionParser::peakOperatorPrecedence() {
    if (stream_.hasMoreTokens() && stream_.nextTokenIs(TokenType::Operator)) {
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
        auto &token = stream_.consumeToken();
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
        case TokenType::DocumentationComment:
            throw CompilerError(token.position(), "Misplaced documentation comment.");
        case TokenType::BlockBegin:
            return parseClosure(token);
        case TokenType::New:
            return parseInitialization(token.position());
        case TokenType::This:
            return std::make_shared<ASTThis>(token.position());
        default:
            throw CompilerError(token.position(), "Unexpected token ", token.stringName(), ".");
    }
}

std::shared_ptr<ASTExpr> FunctionParser::parseExprIdentifier(const Token &token) {
    switch (token.value()[0]) {
        case E_CLOUD:
            return parseUnaryPrefix<ASTIsNothigness>(token);
        case E_TRAFFIC_LIGHT:
            return parseUnaryPrefix<ASTIsError>(token);
        case E_WHITE_LARGE_SQUARE:
            return parseUnaryPrefix<ASTMetaTypeFromInstance>(token);
        case E_BEER_MUG:
            return parseUnaryPrefix<ASTUnwrap>(token);
        case E_METRO:
            return parseUnaryPrefix<ASTUnwrap>(token);
        case E_WHITE_SQUARE_BUTTON: {
            Type t = parseType(typeContext_, TypeDynamism::None);
            return std::make_shared<ASTMetaTypeInstantiation>(t, token.position());
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
        case E_HIGH_VOLTAGE_SIGN:
            return std::make_shared<ASTNothingness>(token.position());
        case E_HOT_PEPPER: {
            if (stream_.consumeTokenIf(E_DOUGHNUT)) {
                auto name = stream_.consumeToken(TokenType::Identifier).value();
                return std::make_shared<ASTCaptureTypeMethod>(name, parseTypeExpr(token.position()), token.position());
            }

            auto name = stream_.consumeToken(TokenType::Identifier).value();
            return std::make_shared<ASTCaptureMethod>(name, parseExpr(0), token.position());
        }
        case E_CHIPMUNK: {
            auto name = stream_.consumeToken(TokenType::Identifier).value();
            return std::make_shared<ASTSuperMethod>(name, parseArguments(token.position()), token.position());
        }
        case E_DOUGHNUT: {
            auto name = stream_.consumeToken(TokenType::Identifier).value();
            auto callee = parseTypeExpr(token.position());
            return std::make_shared<ASTTypeMethod>(name, callee, parseArguments(token.position()), token.position());
        }
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
                                               package_, token.position(), false, std::u32string(), false, false,
                                               FunctionType::Closure);

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
    Type ot = parseType(typeContext_, TypeDynamism::AllKinds);
    switch (ot.type()) {
        case TypeType::GenericVariable:
            throw CompilerError(p, "Generic Arguments are not yet available for reflection.");
        case TypeType::Class:
            return std::make_shared<ASTStaticType>(ot, TypeAvailability::StaticAndAvailabale, p);
        case TypeType::LocalGenericVariable:
            throw CompilerError(p, "Function Generic Arguments are not available for reflection.");
        default:
            break;
    }
    return std::make_shared<ASTStaticType>(ot, TypeAvailability::StaticAndUnavailable, p);
}

}  // namespace EmojicodeCompiler
