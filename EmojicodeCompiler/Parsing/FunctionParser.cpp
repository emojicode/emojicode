//
//  FunctionParser.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "../Lex/Token.hpp"
#include "../AST/ASTBinaryOperator.hpp"
#include "../AST/ASTClosure.hpp"
#include "../AST/ASTControlFlow.hpp"
#include "../AST/ASTInitialization.hpp"
#include "../AST/ASTLiterals.hpp"
#include "../AST/ASTMethod.hpp"
#include "../AST/ASTNode.hpp"
#include "../AST/ASTTypeExpr.hpp"
#include "../AST/ASTUnary.hpp"
#include "../AST/ASTVariables.hpp"
#include "FunctionParser.hpp"

namespace EmojicodeCompiler {

std::shared_ptr<ASTBlock> FunctionParser::parse() {
    while (stream_.nextTokenIsEverythingBut(E_WATERMELON)) {
        fnode_->appendNode(parseStatement());
    }
    stream_.consumeToken();
    return fnode_;
}

ASTBlock FunctionParser::parseBlock() {
    auto &token = stream_.requireIdentifier(E_GRAPES);

    auto block = ASTBlock(token.position());
    while (stream_.nextTokenIsEverythingBut(E_WATERMELON)) {
        block.appendNode(parseStatement());
    }
    stream_.consumeToken();
    return block;
}

std::shared_ptr<ASTExpr> FunctionParser::parseCondition() {
    if (stream_.consumeTokenIf(E_SOFT_ICE_CREAM)) {
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

    stream_.consumeToken(TokenType::BeginArgumentList);
    while (stream_.nextTokenIsEverythingBut(TokenType::EndArgumentList)) {
        args.addArguments(parseExpr(0));
    }
    stream_.consumeToken();
    return args;
}

std::shared_ptr<ASTStatement> FunctionParser::parseStatement() {
    const Token &token = stream_.consumeToken();
    if (token.type() == TokenType::Identifier) {
        switch (token.value()[0]) {
            case E_SHORTCAKE: {
                auto &varName = stream_.consumeToken(TokenType::Variable);

                Type type = parseType(typeContext_, TypeDynamism::AllKinds);
                return std::make_shared<ASTVariableDeclaration>(type, varName.value(), token.position());
            }
            case E_CUSTARD: {
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
            case E_SOFT_ICE_CREAM: {
                auto &varName = stream_.consumeToken(TokenType::Variable);
                return std::make_shared<ASTFrozenDeclaration>(varName.value(), parseExpr(0), token.position());
            }
            case E_TANGERINE: {
                auto node = std::make_shared<ASTIf>(token.position());
                do {
                    node->addCondition(parseCondition());
                    node->addBlock(parseBlock());
                } while (stream_.consumeTokenIf(E_LEMON));

                if (stream_.consumeTokenIf(E_STRAWBERRY)) {
                    node->addBlock(parseBlock());
                }
                return node;
            }
            case E_AVOCADO: {
                auto &variableToken = stream_.consumeToken(TokenType::Variable);
                auto value = parseExpr(0);
                auto valueBlock = parseBlock();

                stream_.requireIdentifier(E_STRAWBERRY);
                auto errorVariableToken = stream_.consumeToken(TokenType::Variable);
                auto errorBlock = parseBlock();
                return std::make_shared<ASTErrorHandler>(value, variableToken.value(), errorVariableToken.value(),
                                                         valueBlock, errorBlock, token.position());
            }
            case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS: {
                auto cond = parseCondition();
                auto block = parseBlock();
                return std::make_shared<ASTRepeatWhile>(cond, block, token.position());
            }
            case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY: {
                auto &variableToken = stream_.consumeToken(TokenType::Variable);
                auto iteratee = parseExpr(0);
                auto block = parseBlock();
                return std::make_shared<ASTForIn>(iteratee, variableToken.value(), block, token.position());
            }
            case E_GOAT: {
                auto &initializerToken = stream_.consumeToken(TokenType::Identifier);
                auto arguments = parseArguments(token.position());
                return std::make_shared<ASTSuperinitializer>(initializerToken.value(), arguments, token.position());
            }
            case E_POLICE_CARS_LIGHT: {
                return std::make_shared<ASTRaise>(parseExpr(0), token.position());
            }
            case E_RED_APPLE: {
                return std::make_shared<ASTReturn>(parseExpr(0), token.position());
            }
        }
    }
    return std::make_shared<ASTExprStatement>(parseExprTokens(token, 0), token.position());
}

int FunctionParser::peakOperatorPrecedence() {
    if (stream_.hasMoreTokens() && stream_.nextTokenIs(TokenType::Operator)) {
        return operatorPrecedence(operatorType(stream_.nextToken().value()));
    }
    return 0;
}

std::shared_ptr<ASTExpr> FunctionParser::parseExprTokens(const Token &token, int precendence) {
    auto left = parseExprLeft(token, precendence);

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
    switch (token.value().front()) {
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
        default:
            switch (token.type()) {
                case TokenType::String:
                    return std::make_shared<ASTStringLiteral>(token.value(), token.position());
                case TokenType::BooleanTrue:
                    return std::make_shared<ASTBooleanTrue>(token.position());
                case TokenType::BooleanFalse:
                    return std::make_shared<ASTBooleanFalse>(token.position());
                case TokenType::Integer: {
                    int64_t value = std::stoll(token.value().utf8(), nullptr, 0);
                    return std::make_shared<ASTNumberLiteral>(value, token.position());
                }
                case TokenType::Double: {
                    double d = std::stod(token.value().utf8());
                    return std::make_shared<ASTNumberLiteral>(d, token.position());
                }
                case TokenType::Symbol:
                    return std::make_shared<ASTSymbolLiteral>(token.value().front(), token.position());
                case TokenType::Variable:
                    return std::make_shared<ASTGetVariable>(token.value(), token.position());
                case TokenType::Identifier:
                    return parseExprIdentifier(token);
                case TokenType::DocumentationComment:
                    throw CompilerError(token.position(), "Misplaced documentation comment.");
                default:
                    throw CompilerError(token.position(), "Unexpected token %s.", token.stringName());
            }
    }
}

std::shared_ptr<ASTExpr> FunctionParser::parseExprIdentifier(const Token &token) {
    switch (token.value()[0]) {
        case E_COOKIE:
            return parseListingLiteral<ASTConcatenateLiteral>(E_COOKIE, token);
        case E_ICE_CREAM:
            return parseListingLiteral<ASTListLiteral>(E_AUBERGINE, token);
        case E_HONEY_POT:
            return parseListingLiteral<ASTDictionaryLiteral>(E_AUBERGINE, token);
        case E_DOG: {
            return std::make_shared<ASTThis>(token.position());
        }
        case E_HIGH_VOLTAGE_SIGN: {
            return std::make_shared<ASTNothingness>(token.position());
        }
        case E_HOT_PEPPER: {
            if (stream_.consumeTokenIf(E_DOUGHNUT)) {
                auto name = stream_.consumeToken(TokenType::Identifier).value();
                return std::make_shared<ASTCaptureTypeMethod>(name, parseTypeExpr(token.position()), token.position());
            }

            auto name = stream_.consumeToken(TokenType::Identifier).value();
            return std::make_shared<ASTCaptureMethod>(name, parseExpr(0), token.position());
        }
        case E_GRAPES: {
            auto function = new Function(EmojicodeString(E_GRAPES), AccessLevel::Public, true, Type::nothingness(),
                                         package_, token.position(), false, EmojicodeString(), false, false,
                                         FunctionType::Function);

            parseArgumentList(function, typeContext_);
            parseReturnType(function, typeContext_);

            function->setAst(FunctionParser(package_, stream_, typeContext_).parse());
            return std::make_shared<ASTClosure>(function, token.position());
        }
        case E_CHIPMUNK: {
            auto name = stream_.consumeToken(TokenType::Identifier).value();
            return std::make_shared<ASTSuperMethod>(name, parseArguments(token.position()), token.position());
        }
        case E_LARGE_BLUE_DIAMOND: {
            auto type = parseTypeExpr(token.position());
            auto name = stream_.consumeToken(TokenType::Identifier).value();
            return std::make_shared<ASTInitialization>(name, type, parseArguments(token.position()), token.position());
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
        case E_LEMON:
        case E_STRAWBERRY:
        case E_WATERMELON:
        case E_AUBERGINE:
        case E_SHORTCAKE:
        case E_CUSTARD:
        case E_SOFT_ICE_CREAM:
        case E_TANGERINE:
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS:
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY:
        case E_GOAT:
        case E_RED_APPLE:
        case E_POLICE_CARS_LIGHT:
        case E_AVOCADO:
            throw CompilerError(token.position(), "Unexpected statement %s.", token.value().utf8().c_str());
    }
}

std::shared_ptr<ASTTypeExpr> FunctionParser::parseTypeExpr(const SourcePosition &p) {
    if (stream_.consumeTokenIf(E_BLACK_LARGE_SQUARE)) {
        return std::make_shared<ASTTypeFromExpr>(parseExpr(kPrefixPrecedence), p);
    }
    if (stream_.consumeTokenIf(E_MEDIUM_BLACK_CIRCLE)) {
        return std::make_shared<ASTInferType>(p);
    }
    Type ot = parseType(typeContext_, TypeDynamism::AllKinds);
    switch (ot.type()) {
        case TypeType::GenericVariable:
            throw CompilerError(p, "Generic Arguments are not yet available for reflection.");
        case TypeType::Class:
            return std::make_shared<ASTStaticType>(ot, TypeAvailability::StaticAndAvailabale, p);
        case TypeType::Self:
            return std::make_shared<ASTThisType>(p);
        case TypeType::LocalGenericVariable:
            throw CompilerError(p, "Function Generic Arguments are not available for reflection.");
        default:
            break;
    }
    return std::make_shared<ASTStaticType>(ot, TypeAvailability::StaticAndUnavailable, p);
}

}  // namespace EmojicodeCompiler
