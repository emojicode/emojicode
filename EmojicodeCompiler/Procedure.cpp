//
//  Procedure.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "EmojicodeCompiler.h"
#include "utf8.h"
#include "Procedure.h"

void Procedure::checkPromises(Procedure *superProcedure, const char *on, Type contextType){
    if(superProcedure->final){
        ecCharToCharStack(this->name, mn);
        compilerError(this->dToken, "%s of %s was marked 🔏.", on, mn);
    }
    if (!this->returnType.compatibleTo(superProcedure->returnType, contextType)) {
        ecCharToCharStack(this->name, mn);
        auto supername = superProcedure->returnType.toString(contextType, true);
        auto thisname = this->returnType.toString(contextType, true);
        compilerError(this->dToken, "Return type %s of %s is not compatible with the return type %s of its %s.", thisname.c_str(), mn, supername.c_str(), on);
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
        if (!superProcedure->arguments[i].type.compatibleTo(this->arguments[i].type, contextType)) {
            auto supertype = superProcedure->arguments[i].type.toString(contextType, true);
            auto thisname = this->arguments[i].type.toString(contextType, true);
            compilerError(superProcedure->dToken, "Type %s of argument %d is not compatible with its %s argument type %s.", thisname.c_str(), i + 1, on, supertype.c_str());
        }
    }
}