//
//  ExpressionPAGs.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 11/04/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ExpressionPAGs_hpp
#define ExpressionPAGs_hpp

class Token;
class TypeExpectation;
class FunctionPAGInterface;
class Type;

Type pagConcatenateLiteral(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag);
Type pagListLiteral(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag);
Type pagDictionaryLiteral(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag);
Type pagRangeLiteral(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag);
Type pagIsNothingness(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag);
Type pagIsError(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag);
Type pagCast(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag);
Type pagUnwrapOptional(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag);
Type pagExtraction(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag);
Type pagCallableCall(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag);
Type pagTypeInstanceFromInstance(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag);
Type pagTypeInstance(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag);
Type pagMethodCapture(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag);
Type pagTypeMethod(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag);
Type pagInstatiation(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag);
Type pagSuperMethod(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag);
Type pagIdentityCheck(const Token &token, const TypeExpectation &expectation, FunctionPAGInterface &functionPag);

#endif /* ExpressionPAGs_hpp */
