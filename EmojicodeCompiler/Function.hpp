//
//  Function.h
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Function_hpp
#define Function_hpp

#include <vector>
#include <map>
#include "Writer.hpp"
#include "Token.hpp"
#include "TokenStream.hpp"
#include "Type.hpp"

enum AccessLevel {
    PUBLIC, PRIVATE, PROTECTED
};

struct Argument {
    Argument(const Token &n, Type t) : name(n), type(t) {}
    
    /** The name of the variable */
    const Token &name;
    /** The type */
    Type type;
};

class Callable {
public:
    Callable(SourcePosition p) : position_(p) {};
    
    std::vector<Argument> arguments;
    /** Return type of the method */
    Type returnType = typeNothingness;
    
    /** Returns the position at which this callable was defined. */
    const SourcePosition& position() const { return position_; }
    
    /** Returns a copy of the token stream intended to be used to parse this callable. */
    TokenStream tokenStream() const { return tokenStream_; }
    void setTokenStream(TokenStream ts) { tokenStream_ = ts; }
    
    /** The type of this callable when used as value. */
    virtual Type type() = 0;
private:
    TokenStream tokenStream_;
    SourcePosition position_;
};

class Closure: public Callable {
public:
    Closure(SourcePosition p) : Callable(p) {};
    Type type();
};

/** Functions are callables that belong to a class as either method, class method or initialiser. */
class Function: public Callable {
public:
    static bool foundStart;
    static Function *start;
    
    static void checkReturnPromise(Type returnThis, Type returnSuper, EmojicodeChar name, SourcePosition position,
                                   const char *on, Type contextType);
    static void checkArgumentCount(size_t thisCount, size_t superCount, EmojicodeChar name, SourcePosition position,
                                   const char *on, Type contextType);
    static void checkArgument(Type thisArgument, Type superArgument, int index, SourcePosition position,
                              const char *on, Type contextType);
    
    Function(EmojicodeChar name, AccessLevel level, bool final, Type owningType,
              Package *package, SourcePosition p, bool overriding, EmojicodeString documentationToken, bool deprecated)
        : Callable(p),
          name(name),
          overriding(overriding),
          deprecated(deprecated),
          access(level),
          owningType(owningType),
          documentationToken(documentationToken),
          package(package) {}
    
    /** The function name. A Unicode code point for an emoji */
    EmojicodeChar name;
    
    /** Whether the method is native. */
    bool native = false;
    bool final = false;
    bool overriding = false;
    bool deprecated = false;
    
    AccessLevel access;
    
    /** Class which defined this function. This can be @c nullptr if the function belongs to a protocol. */
    Type owningType;
    
    EmojicodeString documentationToken;
    
    /** The types for the generic arguments. */
    std::vector<Type> genericArgumentConstraints;
    /** Generic type arguments as variables */
    std::map<EmojicodeString, Type> genericArgumentVariables;
    
    /** The namespace in which the function was defined. This does not necessarily match the class’s namespace. */
    Package *package;
    
    /** Issues a warning on at the given token if the function is deprecated. */
    void deprecatedWarning(const Token &callToken);
    
    /**
     * Check whether this function is breaking promises of @c superFunction.
     */
    void checkPromises(Function *superFunction, const char *on, Type contextType);
    
    void checkOverride(Function *superFunction);
    
    int vti() const { return vti_; }
    void setVti(int vti);
    
    virtual Type type();
    
    const char *on;
private:
    int vti_;
};

class Method: public Function {
    using Function::Function;
public:
    const char *on = "Method";
};

class ClassMethod: public Function {
    using Function::Function;
public:
    const char *on = "Class Method";
};
 
class Initializer: public Function {
public:
    Initializer(EmojicodeChar name, AccessLevel level, bool final, Type owningType, Package *package, SourcePosition p,
                bool overriding, EmojicodeString documentationToken, bool deprecated, bool r, bool crn)
        : Function(name, level, final, owningType, package, p, overriding, documentationToken, deprecated),
          required(r),
          canReturnNothingness(crn) {
              returnType = owningType;
    }
    
    bool required;
    bool canReturnNothingness;
    const char *on = "Initializer";
    
    Type type();
};

#endif /* Function_hpp */
