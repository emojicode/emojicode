//
//  JSONCompilerDelegate.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 25/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef JSONCompilerDelegate_hpp
#define JSONCompilerDelegate_hpp

#include "Compiler.hpp"
#include "JSONHelper.h"

namespace EmojicodeCompiler {

namespace CLI {

/// An CompilerDelegate that prints errors and warnings to stderr as JSON.
class JSONCompilerDelegate : public CompilerDelegate {
    void begin() override;
    void error(const SourcePosition &p, const std::string &message) override;
    void warn(const SourcePosition &p, const std::string &message) override;
    void finish() override;
private:
    CommaPrinter printer_;
    void printJson(const char *type, const SourcePosition &p, const std::string &message);
};

}  // namespace CLI

}  // namespace EmojicodeCompiler

#endif /* JSONCompilerDelegate_hpp */
