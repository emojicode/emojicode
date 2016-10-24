//
//  Callable.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/09/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Callable_hpp
#define Callable_hpp

#include <vector>
#include "Token.hpp"
#include "Type.hpp"
#include "TokenStream.hpp"

struct Argument {
    Argument(const Token &n, Type t) : name(n), type(t) {}
    
    /** The name of the variable */
    const Token &name;
    /** The type */
    Type type;
};

/** A callable is a self-containing piece of code that can be executed, "called", and returns a value. Methods, class 
    methods and closures are examples of callables. */
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
    virtual Type type() const;
private:
    TokenStream tokenStream_;
    SourcePosition position_;
};

#endif /* Callable_hpp */
