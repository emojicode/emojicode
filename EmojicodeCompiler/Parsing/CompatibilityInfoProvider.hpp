//
//  CompatibilityInfoProvider.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef MigArgsCount_hpp
#define MigArgsCount_hpp

#include <map>
#include <vector>

namespace EmojicodeCompiler {

class Function;

/// An CompatibilityInfoProvider object is initialized from an .emojimig file and provides the argument counts
/// to parse arguments in compatiblity mode.
class CompatibilityInfoProvider {
public:
    struct Selection {
    friend CompatibilityInfoProvider;
    private:
        std::vector<unsigned int> *selectedFunction_ = nullptr;
        size_t index_ = 0;
    };

    /// Constructs an CompatibilityInfoProvider instance from the .emojimig file at the given path.
    CompatibilityInfoProvider(const std::string &path);

    void selectFunction(Function *function);
    /// Returns the next argument count for the selected function.
    /// @pre A function must have been previously selected with selectFunction().
    /// @throws CompilerError If there are no more argument counts to return.
    unsigned int nextArgumentsCount();

    Selection selection() { return selection_; }
    void setSelection(Selection selection) { selection_ = std::move(selection); }
private:
    Selection selection_;
    std::map<std::string, std::map<size_t, std::vector<unsigned int>>> files_;
};

}  // namespace EmojicodeCompiler

#endif /* MigArgsCount_hpp */
