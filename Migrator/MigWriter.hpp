//
//  MigWriter.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef MigWriter_hpp
#define MigWriter_hpp

#include <fstream>
#include <map>
#include <vector>

namespace EmojicodeCompiler {

class ArgumentsMigCreator;
class Function;
class Package;

class MigWriter {
public:
    MigWriter(const std::string &outPath, Package *underscore) : out_(outPath, std::ios_base::out),
    underscore_(underscore) {}
    void add(Function *function);
    void finish();
private:
    std::map<std::string, std::vector<Function *>> functions_;
    std::fstream out_;
    Package *underscore_;
};

}

#endif /* MigWriter_hpp */
