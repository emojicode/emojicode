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
    Variable(Type type, uint8_t id, bool initd, bool frozen, const Token &token)
        : type(type), id(id), initialized(initd), definitionToken(token), frozen_(frozen) {};
    /** The type of the variable. */
    Type type;
    /** The ID of the variable. */
    uint8_t id;
    /** The variable is initialized if this field is greater than 0. */
    int initialized;
    /** The variable name token which defined this variable. */
    const Token &definitionToken;
    
    /** Throws an error if the variable is not initalized. */
    void uninitalizedError(const Token &variableToken) const;
    /** Marks the variable as mutated or issues an error if the variable is frozen. */
    void mutate(const Token &variableToken);
    
    bool mutated() const { return mutated_; }
    bool frozen() const { return frozen_; }
private:
    /** Indicating whether variable was frozen. */
    bool frozen_;
    /** Mutated */
    bool mutated_ = false;
};

#endif /* Variable_hpp */
