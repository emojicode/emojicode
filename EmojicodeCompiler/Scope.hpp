//
//  Scope.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Scope_hpp
#define Scope_hpp

#include <map>
#include <memory>
#include "Token.hpp"
#include "Variable.hpp"

class CallableScoper;

class Scope {
    friend CallableScoper;
public:
    Scope() {};
    
    void setVariableInitialization(bool initd);
    void pushInitializationLevel();
    void popInitializationLevel();
    
    /** Sets a variable in this scope. */
    void setLocalVariable(const EmojicodeString &variable, Variable value);
    
    /**
     * Retrieves a variable form the scope or returns @c nullptr.
     */
    Variable& getLocalVariable(const EmojicodeString &variable);

    bool hasLocalVariable(const EmojicodeString &variable) const;
    
    /**
     * Emits @c errorMessage if not all instance variable were initialized.
     * @params errorMessage The error message that will probably be issued. It should include @c %s for the name
                            of the variable.
     */
    void initializerUnintializedVariablesCheck(const Token &errorToken, const char *errorMessage);
    
    /**
     * Emits a warning for each non-frozen variable that has not been mutated.
     */
    void recommendFrozenVariables() const;
    
    size_t localVariableCount() const { return map_.size(); }
private:
    std::map<EmojicodeString, Variable> map_;
};

#endif /* Scope_hpp */
