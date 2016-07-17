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

enum class AccessLevel {
    Public, Private, Protected
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
          final_(final),
          overriding_(overriding),
          deprecated_(deprecated),
          access_(level),
          owningType_(owningType),
          package_(package),
          documentation_(documentationToken) {}
    
    /** The function name. A Unicode code point for an emoji */
    EmojicodeChar name;
    
    /** Whether the method is implemented natively and Run-Time Native Linking must occur. */
    bool native = false;
    /** Whether the method was marked as final and can’t be overriden. */
    bool final() const { return final_; }
    /** Whether the method is intended to override a super method. */
    bool overriding() const { return overriding_; }
    /** Whether the method is deprecated. */
    bool deprecated() const { return deprecated_; }
    /** Returns the access level to this method. */
    AccessLevel accessLevel() const { return access_; }
    
    /** Type to which this function belongs. 
        This can be Nothingness if the function doesn’t belong to any type (e.g. 🏁). */
    Type owningType() const { return owningType_; }
    
    const EmojicodeString& documentation() const { return documentation_; }
    
    /** The types for the generic arguments. */
    std::vector<Type> genericArgumentConstraints;
    /** Generic type arguments as variables */
    std::map<EmojicodeString, Type> genericArgumentVariables;
    
    /** The namespace in which the function was defined.
        This does not necessarily match the package of @c owningType. */
    Package* package() const { return package_; }
    
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
    bool final_;
    bool overriding_;
    bool deprecated_;
    AccessLevel access_;
    Type owningType_;
    Package *package_;
    EmojicodeString documentation_;
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
