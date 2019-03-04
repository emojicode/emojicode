//
//  Error.h
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 11/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Error_h
#define Error_h

#include "String.h"
#include "../runtime/Runtime.h"

namespace s {

class Error : public runtime::Object<Error>  {
public:
    Error(const char *message);

private:
    s::String *message;
    runtime::SimpleOptional<s::String*> location = runtime::NoValue;
};

class IOError : public runtime::Object<IOError>  {
public:
    IOError(const char *message);
    /// Creates an IOError from errno
    IOError();

private:
    s::String *message;
    runtime::SimpleOptional<s::String*> location = runtime::NoValue;
};

}  // namespace s

SET_INFO_FOR(s::Error, s, 1f6a7)
SET_INFO_FOR(s::IOError, s, 1f6a7_1f538_2195)

#define EJC_COND_RAISE_IO(cond, raiser) if (!(cond)) { EJC_RAISE(raiser, s::IOError::init()); }
#define EJC_COND_RAISE_IO_VOID(cond, raiser) if (!(cond)) { EJC_RAISE_VOID(raiser, s::IOError::init()); }

#endif /* Error.h */

