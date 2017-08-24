//
//  InformationDesk.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 17/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "InformationDesk.hpp"

#include "EmojicodeCompiler.hpp"
#include "Lex/Lexer.hpp"
#include <algorithm>
#include <iostream>

namespace EmojicodeCompiler {

//enum class InformationDeskFunction {
//    Method, TypeMethod, Initializer
//};
//
//void InformationDesk::sizeOfVariable(const std::string &string) {
//    stream_ = lexString(string, SourcePosition(0, 0, "<CLI>"));
//
//    auto f = InformationDeskFunction::Method;
//    if (stream_.consumeTokenIf(E_LARGE_BLUE_DIAMOND)) {
//        f = InformationDeskFunction::Initializer;
//    }
//    else if (stream_.consumeTokenIf(E_DOUGHNUT)) {
//        f = InformationDeskFunction::TypeMethod;
//    }
//
//    auto parsedType = parseTypeIdentifier();
//    Type type = Type::nothingness();
//    if (!package_->fetchRawType(parsedType, false, &type)) {
//        throw "no type";
//    }
//
//    auto name = stream_.consumeToken(TokenType::Identifier);
//    Function* function;
//    switch (f) {
//        case InformationDeskFunction::Initializer:
//            function = type.typeDefinition()->lookupInitializer(name.value());
//            break;
//        case InformationDeskFunction::Method:
//            function = type.typeDefinition()->lookupMethod(name.value());
//            break;
//        case InformationDeskFunction::TypeMethod:
//            function = type.typeDefinition()->lookupTypeMethod(name.value());
//            break;
//    }
//
//    if (function == nullptr) {
//        throw "no funciton";
//    }
//
//    auto variableName = stream_.consumeToken(TokenType::Variable);
//
//    Argument *theArg = nullptr;
//    size_t index = 0;
//    for (auto &arg : function->arguments) {
//        if (arg.variableName == variableName.value()) {
//            theArg = &arg;
//            break;
//        }
//        index += arg.type.size();
//    }
//
//    if (theArg == nullptr) {
//        throw "no arg";
//    }
//    std::cout << "ℹ️ Variable " << utf8(variableName.value()) << " is " << theArg->type.size() << " words large ";
//    std::cout << "and has index " << index << "\n";
//}

}  // namespace EmojicodeCompiler
