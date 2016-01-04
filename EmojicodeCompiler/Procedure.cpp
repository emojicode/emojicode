//
//  Procedure.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "EmojicodeCompiler.h"

template <typename T> void Procedure::duplicateDeclarationCheck(std::map<EmojicodeChar, T> dict){
    if (dict.count(name)) {
        ecCharToCharStack(name, nameString);
        compilerError(dToken, "%s %s is declared twice.", on, nameString);
    }
}

void Procedure::checkPromises(Procedure *superProcedure, Type parentType){
    if(superProcedure->final){
        ecCharToCharStack(this->name, mn);
        compilerError(this->dToken, "%s of %s was marked 🔏.", on, mn);
    }
    if (!typesCompatible(this->returnType, superProcedure->returnType, parentType)) {
        ecCharToCharStack(this->name, mn);
        char *supername = typeToString(superProcedure->returnType, parentType, true);
        char *thisname = typeToString(this->returnType, parentType, true);
        compilerError(this->dToken, "Return type %s of %s is not compatible with the return type %s of its %s.", thisname, mn, supername, on);
    }
    if (this->arguments.size() > 0) {
        ecCharToCharStack(this->name, mn);
        compilerError(this->dToken, "%s expects arguments but its %s doesn't.", mn, on);
    }
    if(superProcedure->arguments.size() != this->arguments.size()){
        ecCharToCharStack(this->name, mn);
        compilerError(this->dToken, "%s expects %s arguments than its %s.", mn, (superProcedure->arguments.size() < this->arguments.size()) ? "more" : "less", on);
    }
    for (uint8_t i = 0; i < superProcedure->arguments.size(); i++) {
        //other way, because the method may define a more generic type
        if (!typesCompatible(superProcedure->arguments[i].type, this->arguments[i].type, parentType)) {
            char *supertype = typeToString(superProcedure->arguments[i].type, parentType, true);
            char *thisname = typeToString(this->arguments[i].type, parentType, true);
            compilerError(superProcedure->dToken, "Type %s of argument %d is not compatible with its %s argument type %s.", thisname, i + 1, on, supertype);
        }
    }
}