//
//  Variable.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Variable_hpp
#define Variable_hpp

#include "Type.hpp"
#include "Token.hpp"

class Variable {
public:
    Variable(Type type, int id, bool initd, bool frozen, const Token &token)
        : type(type), initialized(initd), definitionToken(token), frozen_(frozen), id_(id) {};
    /** The type of the variable. */
    Type type;
    /** The ID of the variable. */
    int id() const { return id_; }
    void setId(int id) { id_ = id; }
    /** The variable is initialized if this field is greater than 0. */
    int initialized;
    /** The variable name token which defined this variable. */
    const Token &definitionToken;
    
    /** Throws an error if the variable is not initalized. */
    void uninitalizedError(const Token &variableToken) const;
    /** Marks the variable as mutated or issues an error if the variable is frozen. */
    void mutate(const Token &variableToken);
    
    /** Whether the variable was mutated since its definition. */
    bool mutated() const { return mutated_; }
    /** Whether this is a frozen variable. */
    bool frozen() const { return frozen_; }
private:
    bool frozen_;
    bool mutated_ = false;
    
    int id_;
};

#endif /* Variable_hpp */
