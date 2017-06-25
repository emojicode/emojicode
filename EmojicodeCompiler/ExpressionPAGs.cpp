//
//  ExpressionPAGs.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 11/04/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ExpressionPAGs.hpp"
#include "../EmojicodeInstructions.h"
#include "Class.hpp"
#include "CompilerError.hpp"
#include "Enum.hpp"
#include "FunctionPAGInterface.hpp"
#include "Protocol.hpp"
#include "Type.hpp"
#include "TypeExpectation.hpp"
#include "ValueType.hpp"
#include <cassert>

Type pagConcatenateLiteral(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag) {
    functionPag.box(expectation, Type(CL_STRING, false));
    functionPag.writer().writeInstruction(INS_OPT_STRING_CONCATENATE_LITERAL);
    auto placeholder = functionPag.writer().writeInstructionPlaceholder();

    int stringCount = 0;

    while (functionPag.stream().nextTokenIsEverythingBut(E_COOKIE)) {
        functionPag.parseTypeSafeExpr(Type(CL_STRING, false));
        stringCount++;
    }
    functionPag.stream().consumeToken(TokenType::Identifier);

    if (stringCount == 0) {
        throw CompilerError(token.position(), "An empty 🍪 is invalid.");
    }

    placeholder.write(stringCount);
    return Type(CL_STRING, false);
}

Type pagListLiteral(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag) {
    functionPag.box(expectation, Type(CL_LIST, false));
    functionPag.writer().writeInstruction(INS_OPT_LIST_LITERAL);

    auto &scope = functionPag.scoper().pushScope();

    auto variablePlaceholder = functionPag.writer().writeInstructionPlaceholder();
    auto placeholder = functionPag.writer().writeInstructionsCountPlaceholderCoin();

    bool hasInitCount = false;
    InstructionCount initCount = 0;

    Type type = Type(CL_LIST, false);
    if (expectation.type() == TypeContent::Class && expectation.eclass() == CL_LIST) {
        auto elementType = Type(TypeContent::GenericVariable, false, 0, expectation.eclass()).resolveOn(expectation);
        while (functionPag.stream().nextTokenIsEverythingBut(E_AUBERGINE)) {
            functionPag.parseTypeSafeExpr(elementType);
            if (!hasInitCount) {
                initCount = functionPag.writer().count();
                hasInitCount = true;
            }
        }
        functionPag.stream().consumeToken(TokenType::Identifier);
        type.setGenericArgument(0, elementType);
        auto &var = scope.setLocalVariable(EmojicodeString(), elementType, true, token.position());
        if (hasInitCount) {
            var.initialize(initCount - 1);
        }
        variablePlaceholder.write(var.id());
    }
    else {
        CommonTypeFinder ct;
        while (functionPag.stream().nextTokenIsEverythingBut(E_AUBERGINE)) {
            ct.addType(functionPag.parseExpr(TypeExpectation(false, true, false)), functionPag.typeContext());
            if (!hasInitCount) {
                initCount = functionPag.writer().count();
                hasInitCount = true;
            }
        }
        functionPag.stream().consumeToken(TokenType::Identifier);
        type.setGenericArgument(0, ct.getCommonType(token.position()));
        auto varType = Type(TypeContent::GenericVariable, false, 0, CL_LIST).resolveOn(type);
        auto &var = scope.setLocalVariable(EmojicodeString(), varType, true, token.position());
        var.initialize(initCount);
        if (hasInitCount) {
            var.initialize(initCount - 1);
        }
        variablePlaceholder.write(var.id());
    }

    placeholder.write();
    functionPag.popScope();
    return type;
}

Type pagDictionaryLiteral(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag) {
    functionPag.box(expectation, Type(CL_DICTIONARY, false));
    functionPag.writer().writeInstruction(INS_OPT_DICTIONARY_LITERAL);

    auto &scope = functionPag.scoper().pushScope();

    auto variablePlaceholder = functionPag.writer().writeInstructionPlaceholder();
    auto placeholder = functionPag.writer().writeInstructionsCountPlaceholderCoin();

    auto initCount = functionPag.writer().count();

    Type type = Type(CL_DICTIONARY, false);
    if (expectation.type() == TypeContent::Class && expectation.eclass() == CL_DICTIONARY) {
        auto elementType = Type(TypeContent::GenericVariable, false, 0, expectation.eclass()).resolveOn(expectation);
        while (functionPag.stream().nextTokenIsEverythingBut(E_AUBERGINE)) {
            functionPag.parseTypeSafeExpr(Type(CL_STRING, false));
            functionPag.parseTypeSafeExpr(elementType);
        }
        functionPag.stream().consumeToken(TokenType::Identifier);
        type.setGenericArgument(0, elementType);
        auto &var = scope.setLocalVariable(EmojicodeString(), elementType, true, token.position());
        var.initialize(initCount - 1);
        variablePlaceholder.write(var.id());
    }
    else {
        CommonTypeFinder ct;
        while (functionPag.stream().nextTokenIsEverythingBut(E_AUBERGINE)) {
            functionPag.parseTypeSafeExpr(Type(CL_STRING, false));
            ct.addType(functionPag.parseExpr(TypeExpectation(false, true, false)), functionPag.typeContext());
        }
        functionPag.stream().consumeToken(TokenType::Identifier);
        type.setGenericArgument(0, ct.getCommonType(token.position()));
        auto &var = scope.setLocalVariable(EmojicodeString(), type.genericArguments()[0], true, token.position());
        var.initialize(initCount - 1);
        variablePlaceholder.write(var.id());
    }

    placeholder.write();
    functionPag.popScope();
    return type;
}

Type pagRangeLiteral(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag) {
    Type type = Type::nothingness();
    functionPag.package()->fetchRawType(EmojicodeString(E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE), globalNamespace,
                                        false, token.position(), &type);

    functionPag.box(expectation, type, functionPag.writer());

    functionPag.writer().writeInstruction(INS_INIT_VT);
    auto initializer = type.valueType()->getInitializer(token, type, functionPag.typeContext());
    functionPag.writer().writeInstruction(initializer->vtiForUse());

    functionPag.parseFunctionCall(type, initializer, token);

    return type;
}

Type pagIsNothingness(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag) {
    functionPag.box(expectation, Type::boolean());
    functionPag.writer().writeInstruction(INS_IS_NOTHINGNESS);
    auto type = functionPag.parseExpr(TypeExpectation(true, false));
    if (!type.optional() && type.type() != TypeContent::Something) {
        throw CompilerError(token.position(), "☁️ can only be used with optionals and ⚪️.");
    }
    return Type::boolean();
}

Type pagIsError(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag) {
    functionPag.box(expectation, Type::boolean());
    functionPag.writer().writeInstruction(INS_IS_ERROR);
    auto type = functionPag.parseExpr(TypeExpectation(true, false));
    if (type.type() != TypeContent::Error) {
        throw CompilerError(token.position(), "🚥 can only be used with 🚨.");
    }
    return Type::boolean();
}

Type pagCast(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag) {
    auto insertionPoint = functionPag.writer().getInsertionPoint();
    auto placeholder = functionPag.writer().writeInstructionPlaceholder();

    Type originalType = functionPag.parseExpr(TypeExpectation(false, false));
    auto pair = functionPag.parseTypeAsValue(token.position(), TypeExpectation());
    auto type = pair.first.resolveOnSuperArgumentsAndConstraints(functionPag.typeContext());

    if (originalType.compatibleTo(type, functionPag.typeContext())) {
        throw CompilerError(token.position(), "Unnecessary cast.");
    }
    else if (!type.compatibleTo(originalType, functionPag.typeContext())) {
        auto typeString = type.toString(functionPag.typeContext(), true);
        throw CompilerError(token.position(), "Cast to unrelated type %s will always fail.",
                            typeString.c_str());
    }

    if (type.type() == TypeContent::Class) {
        auto offset = type.eclass()->numberOfGenericArgumentsWithSuperArguments() - type.eclass()->ownGenericArgumentVariables().size();
        for (size_t i = 0; i < type.eclass()->ownGenericArgumentVariables().size(); i++) {
            if (!type.eclass()->genericArgumentConstraints()[offset + i].compatibleTo(type.genericArguments()[i], type) ||
                !type.genericArguments()[i].compatibleTo(type.eclass()->genericArgumentConstraints()[offset + i], type)) {
                throw CompilerError(token.position(), "Dynamic casts involving generic type arguments are not "
                                    "possible yet. Please specify the generic argument constraints of the "
                                    "class for compatibility with future versions.");
            }
        }

        if (originalType.type() == TypeContent::Someobject || originalType.type() == TypeContent::Class) {
            placeholder.write(INS_DOWNCAST_TO_CLASS);
            if (originalType.optional()) {
                throw CompilerError(token.position(), "Downcast on classes with optionals not possible.");
            }
            assert(originalType.storageType() == StorageType::Simple && originalType.size() == 1);
        }
        else {
            assert(originalType.storageType() == StorageType::Box);
            placeholder.write(INS_CAST_TO_CLASS);
        }
    }
    else if (type.type() == TypeContent::Protocol && isStatic(pair.second)) {
        if (!type.genericArguments().empty()) {
            throw CompilerError(token.position(), "Cannot cast to generic protocols.");
        }
        assert(originalType.storageType() == StorageType::Box);
        placeholder.write(INS_CAST_TO_PROTOCOL);
        functionPag.writer().writeInstruction(type.protocol()->index);
    }
    else if ((type.type() == TypeContent::ValueType || type.type() == TypeContent::Enum)
             && isStatic(pair.second)) {
        assert(originalType.storageType() == StorageType::Box);
        placeholder.write(INS_CAST_TO_VALUE_TYPE);
        functionPag.writer().writeInstruction(type.boxIdentifier());
        type.forceBox();
    }
    else {
        auto typeString = pair.first.toString(functionPag.typeContext(), true);
        throw CompilerError(token.position(), "You cannot cast to %s.", typeString.c_str());
    }

    type.setOptional();
    functionPag.box(expectation, type, insertionPoint);
    return type;
}

Type pagUnwrapOptional(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag) {
    auto insertionPoint = functionPag.writer().getInsertionPoint();
    auto placeholder = functionPag.writer().writeInstructionPlaceholder();

    Type t = functionPag.parseExpr(TypeExpectation(true, false));

    if (!t.optional()) {
        throw CompilerError(token.position(), "🍺 can only be used with optionals.");
    }

    assert(t.isReference());
    t.setReference(false);

    if (t.storageType() == StorageType::Box) {
        placeholder.write(INS_UNWRAP_BOX_OPTIONAL);
        t.setOptional(false);
    }
    else {
        placeholder.write(INS_UNWRAP_SIMPLE_OPTIONAL);
        t.setOptional(false);
        functionPag.writer().writeInstruction(t.size());
    }
    functionPag.box(expectation, t, insertionPoint);
    return t;
}

Type pagExtraction(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag) {
    auto insertionPoint = functionPag.writer().getInsertionPoint();
    auto placeholder = functionPag.writer().writeInstructionPlaceholder();

    Type t = functionPag.parseExpr(TypeExpectation(true, false));

    if (t.type() != TypeContent::Error) {
        throw CompilerError(token.position(), "🚇 can only be used with 🚨.");
    }

    assert(t.isReference());
    t.setReference(false);

    Type type = t.genericArguments()[1];

    if (t.storageType() == StorageType::Box) {
        placeholder.write(INS_ERROR_CHECK_BOX_OPTIONAL);
        type.forceBox();
    }
    else {
        placeholder.write(INS_ERROR_CHECK_SIMPLE_OPTIONAL);
        functionPag.writer().writeInstruction(type.size());
    }
    functionPag.box(expectation, type, insertionPoint);
    return type;
}

Type pagCallableCall(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag) {
    functionPag.makeEffective();
    auto insertionPoint = functionPag.writer().getInsertionPoint();
    functionPag.writer().writeInstruction(INS_EXECUTE_CALLABLE);

    Type type = functionPag.parseExpr(TypeExpectation(false, false, false));

    if (type.type() != TypeContent::Callable) {
        throw CompilerError(token.position(), "Given value is not callable.");
    }

    for (size_t i = 1; i < type.genericArguments().size(); i++) {
        functionPag.writer().writeInstruction(type.genericArguments()[i].size());
        functionPag.parseTypeSafeExpr(type.genericArguments()[i]);
    }

    auto rtype = type.genericArguments()[0];
    functionPag.box(expectation, rtype, insertionPoint);
    return rtype;
}

Type pagTypeInstance(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag) {
    auto insertionPoint = functionPag.writer().getInsertionPoint();
    functionPag.writer().writeInstruction(INS_GET_CLASS_FROM_INDEX);
    Type originalType = functionPag.parseTypeDeclarative(functionPag.typeContext(), TypeDynamism::None);
    if (!originalType.allowsMetaType()) {
        auto string = originalType.toString(functionPag.typeContext(), true);
        throw CompilerError(token.position(), "Can’t get metatype of %s.", string.c_str());
    }
    functionPag.writer().writeInstruction(originalType.eclass()->index);
    originalType.setMeta(true);
    functionPag.box(expectation, originalType, insertionPoint);
    return originalType;
}

Type pagMethodCapture(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag) {
    functionPag.box(expectation, Type::callableIncomplete());
    Function *function;

    auto placeholder = functionPag.writer().writeInstructionPlaceholder();
    if (functionPag.stream().consumeTokenIf(E_DOUGHNUT)) {
        auto &methodName = functionPag.stream().consumeToken();
        auto pair = functionPag.parseTypeAsValue(token.position(), TypeExpectation());
        auto type = pair.first.resolveOnSuperArgumentsAndConstraints(functionPag.typeContext());

        if (type.type() == TypeContent::Class) {
            placeholder.write(INS_CAPTURE_TYPE_METHOD);
        }
        else if (type.type() == TypeContent::ValueType) {
            notStaticError(pair.second, token.position(), "Value Types");
            placeholder.write(INS_CAPTURE_CONTEXTED_FUNCTION);
            functionPag.writer().writeInstruction(INS_GET_NOTHINGNESS);
        }
        else {
            throw CompilerError(token.position(), "You can’t capture method calls on this kind of type.");
        }

        function = type.eclass()->getTypeMethod(methodName, type, functionPag.typeContext());
    }
    else {
        auto &methodName = functionPag.stream().consumeToken();
        Type type = functionPag.parseExpr(TypeExpectation(false, false, false));

        if (type.type() == TypeContent::Class) {
            placeholder.write(INS_CAPTURE_METHOD);
        }
        else if (type.type() == TypeContent::ValueType) {
            placeholder.write(INS_CAPTURE_CONTEXTED_FUNCTION);
            if (type.size() > 1) {
                throw CompilerError(token.position(), "Type not eligible for method capturing.");  // TODO: Improve
            }
        }
        else {
            throw CompilerError(token.position(), "You can’t capture method calls on this kind of type.");
        }
        function = type.typeDefinitionFunctional()->getMethod(methodName, type, functionPag.typeContext());
    }
    functionPag.checkAccessLevel(function, token.position());
    function->deprecatedWarning(token.position());
    functionPag.writer().writeInstruction(function->vtiForUse());
    return function->type();
}

Type pagTypeMethod(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag) {
    functionPag.makeEffective();
    auto insertionPoint = functionPag.writer().getInsertionPoint();
    auto &methodToken = functionPag.stream().consumeToken(TokenType::Identifier);

    auto placeholder = functionPag.writer().writeInstructionPlaceholder();
    auto pair = functionPag.parseTypeAsValue(token.position(), TypeExpectation());
    auto type = pair.first.resolveOnSuperArgumentsAndConstraints(functionPag.typeContext());

    if (type.optional()) {
        compilerWarning(token.position(), "You cannot call optionals on 🍬.");
    }

    Function *method;
    if (type.type() == TypeContent::Class) {
        placeholder.write(INS_DISPATCH_TYPE_METHOD);
        method = type.typeDefinitionFunctional()->getTypeMethod(methodToken, type, functionPag.typeContext());
        functionPag.writer().writeInstruction(method->vtiForUse());
    }
    else if ((type.type() == TypeContent::ValueType || type.type() == TypeContent::Enum)
             && isStatic(pair.second)) {
        method = type.typeDefinitionFunctional()->getTypeMethod(methodToken, type, functionPag.typeContext());
        placeholder.write(INS_CALL_FUNCTION);
        functionPag.writer().writeInstruction(method->vtiForUse());
    }
    else {
        throw CompilerError(token.position(), "You can’t call type methods on %s.",
                            pair.first.toString(functionPag.typeContext(), true).c_str());
    }
    auto rtype = functionPag.parseFunctionCall(type, method, token);
    functionPag.box(expectation, rtype, insertionPoint);
    return rtype;
}

Type pagInstatiation(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag) {
    auto insertionPoint = functionPag.writer().getInsertionPoint();
    auto placeholder = functionPag.writer().writeInstructionPlaceholder();
    auto pair = functionPag.parseTypeAsValue(token.position(), expectation);
    auto type = pair.first.resolveOnSuperArgumentsAndConstraints(functionPag.typeContext());

    auto &initializerName = functionPag.stream().consumeToken(TokenType::Identifier);

    if (type.optional()) {
        throw CompilerError(token.position(), "Optionals cannot be instantiated.");
    }

    if (type.type() == TypeContent::Enum) {
        notStaticError(pair.second, token.position(), "Enums");
        placeholder.write(INS_GET_32_INTEGER);

        auto v = type.eenum()->getValueFor(initializerName.value());
        if (!v.first) {
            throw CompilerError(initializerName.position(), "%s does not have a member named %s.",
                                type.eenum()->name().utf8().c_str(), initializerName.value().utf8().c_str());
        }
        else if (v.second > UINT32_MAX) {
            functionPag.writer().writeInstruction(v.second >> 32);
            functionPag.writer().writeInstruction(static_cast<EmojicodeInstruction>(v.second));
        }
        else {
            functionPag.writer().writeInstruction(static_cast<EmojicodeInstruction>(v.second));
        }
        type.unbox();
        functionPag.box(expectation, type, insertionPoint);
        return type;
    }

    Initializer *initializer;
    if (type.type() == TypeContent::ValueType) {
        notStaticError(pair.second, token.position(), "Value Types");

        placeholder.write(INS_INIT_VT);

        initializer = type.valueType()->getInitializer(initializerName, type, functionPag.typeContext());
        functionPag.writer().writeInstruction(initializer->vtiForUse());

        functionPag.parseFunctionCall(type, initializer, token);
    }
    else if (type.type() == TypeContent::Class) {
        placeholder.write(INS_NEW_OBJECT);

        initializer = type.eclass()->getInitializer(initializerName, type, functionPag.typeContext());

        if (!isStatic(pair.second) && !initializer->required()) {
            throw CompilerError(initializerName.position(),
                                "Only required initializers can be used with dynamic types.");
        }

        initializer->deprecatedWarning(initializerName.position());

        functionPag.writer().writeInstruction(initializer->vtiForUse());

        functionPag.parseFunctionCall(type, initializer, token);
    }
    else {
        throw CompilerError(token.position(), "The given type cannot be instantiated.");
    }

    auto constructedType = initializer->constructedType(type);
    functionPag.box(expectation, constructedType, insertionPoint);
    return constructedType;
}

Type pagTypeInstanceFromInstance(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag) {
    auto insertionPoint = functionPag.writer().getInsertionPoint();
    functionPag.writer().writeInstruction(INS_GET_CLASS_FROM_INSTANCE);
    Type originalType = functionPag.parseExpr(TypeExpectation(false, false, false));
    if (!originalType.allowsMetaType()) {
        auto string = originalType.toString(functionPag.typeContext(), true);
        throw CompilerError(token.position(), "Can’t get metatype of %s.", string.c_str());
    }
    originalType.setMeta(true);
    functionPag.box(expectation, originalType, insertionPoint);
    return originalType;
}

Type pagSuperMethod(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag) {
    functionPag.makeEffective();
    auto insertionPoint = functionPag.writer().getInsertionPoint();
    auto &nameToken = functionPag.stream().consumeToken(TokenType::Identifier);

    if (functionPag.mode() != FunctionPAGMode::ObjectMethod) {
        throw CompilerError(token.position(), "Not within an object-context.");
    }

    Class *superclass = functionPag.typeContext().calleeType().eclass()->superclass();

    if (superclass == nullptr) {
        throw CompilerError(token.position(), "Class has no superclass.");
    }

    Function *method = superclass->getMethod(nameToken, Type(superclass, false), functionPag.typeContext());

    functionPag.writer().writeInstruction(INS_DISPATCH_SUPER);
    functionPag.writer().writeInstruction(INS_GET_CLASS_FROM_INDEX);
    functionPag.writer().writeInstruction(superclass->index);
    functionPag.writer().writeInstruction(method->vtiForUse());

    Type type = functionPag.parseFunctionCall(functionPag.typeContext().calleeType(), method, token);
    functionPag.box(expectation, type, insertionPoint);
    return type;
}

Type pagIdentityCheck(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag) {
    functionPag.box(expectation, Type::boolean());
    functionPag.writer().writeInstruction(INS_SAME_OBJECT);
    
    functionPag.parseTypeSafeExpr(Type::someobject());
    functionPag.parseTypeSafeExpr(Type::someobject());
    
    return Type::boolean();
}
