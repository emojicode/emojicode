//
//  HRFCompilerDelegate.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 25/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef HRFCompilerDelegate_hpp
#define HRFCompilerDelegate_hpp

#include "Compiler.hpp"

namespace EmojicodeCompiler {

namespace CLI {

/// An CompilerDelegate that prints errors and warnings to stderr in a **h**uman **r**eadable **f**ormat.
class HRFCompilerDelegate : public CompilerDelegate {
    void begin() override {}
    void error(const SourcePosition &p, const std::string &message) override;
    void warn(const SourcePosition &p, const std::string &message) override;
    void finish() override {}
};

}  // namespace CLI

}  // namespace EmojicodeCompiler

#endif /* HRFCompilerDelegate_hpp */
