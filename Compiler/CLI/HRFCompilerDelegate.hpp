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
public:
    explicit HRFCompilerDelegate(bool forceColor);

    void begin() override {}
    void error(Compiler *compiler, const std::string &message, const SourcePosition &p) override;
    void warn(Compiler *compiler, const std::string &message, const SourcePosition &p) override;
    void finish() override {}

private:
    void printPosition(const SourcePosition &p) const;

    void printOffendingCode(Compiler *compiler, const SourcePosition &position);

    void printMessage(const std::string &message) const;
};

}  // namespace CLI

}  // namespace EmojicodeCompiler

#endif /* HRFCompilerDelegate_hpp */
