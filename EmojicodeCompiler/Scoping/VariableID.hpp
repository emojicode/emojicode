//
//  VariableID.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 21/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef VariableID_hpp
#define VariableID_hpp

namespace EmojicodeCompiler {

class Scope;
class CGScoper;
class FnCodeGenerator;
class CapturingSemanticScoper;

/// A VariableID is an unique identifier identifying variable in its scope.
/// During semantic analysis a VariableID is obtained for each programmer supplied variable name. The variable names
/// can then be discarded.
/// @attention The VariableID can be thought of like a different name for the variable. It is not a stack
/// location of the variable.
class VariableID {
    friend Scope;
    friend CGScoper;
    friend FnCodeGenerator;
    friend CapturingSemanticScoper;
public:
    VariableID() : id_(4294967295) {}
private:
    explicit VariableID(unsigned int id) : id_(id) {}
    unsigned int id_;
};
    
}

#endif /* VariableID_hpp */
