//
//  HRFApplicationDelegate.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 25/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef HRFApplicationDelegate_hpp
#define HRFApplicationDelegate_hpp

#include "../Application.hpp"

namespace EmojicodeCompiler {

namespace CLI {

/// An ApplicationDelegate that prints errors and warnings to stderr in a **h**uman **r**eadable **f**ormat.
class HRFApplicationDelegate : public ApplicationDelegate {
    void begin() override {}
    void error(const SourcePosition &p, const std::string &message) override;
    void warn(const SourcePosition &p, const std::string &message) override;
    void finish() override {}
};

}  // namespace CLI

}  // namespace EmojicodeCompiler

#endif /* HRFApplicationDelegate_hpp */
