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
#define RAPIDJSON_HAS_STDSTRING 1
#include "Utils/rapidjson/writer.h"
#include "Utils/rapidjson/ostreamwrapper.h"

namespace EmojicodeCompiler {

namespace CLI {

/// An CompilerDelegate that prints errors and warnings to stderr as JSON.
class JSONCompilerDelegate : public CompilerDelegate {
public:
    JSONCompilerDelegate();

    void begin() override;
    void error(Compiler *compiler, const CompilerError &ce) override;
    void warn(Compiler *compiler, const std::string &message, const SourcePosition &p) override;
    void finish() override;
private:
    rapidjson::OStreamWrapper wrapper_;
    rapidjson::Writer<rapidjson::OStreamWrapper> writer_;
    void printJson(const char *type, const SourcePosition &p, const std::string &message);
};

}  // namespace CLI

}  // namespace EmojicodeCompiler

#endif /* JSONCompilerDelegate_hpp */
