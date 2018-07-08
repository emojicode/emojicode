//
// Created by Theo Weidmann on 02.06.18.
//

#ifndef EMOJICODE_MFANALYSER_HPP
#define EMOJICODE_MFANALYSER_HPP

namespace EmojicodeCompiler {

class Package;
class Function;
class TypeDefinition;

class MFAnalyser {
public:
    MFAnalyser(Package *package) : package_(package) {}
    void analyse();

private:
    Package *package_;
    void analyseTypeDefinition(TypeDefinition *typeDef);
    void analyseFunction(Function *function);
};

}  // namespace EmojicodeCompiler

#endif //EMOJICODE_MFANALYSER_HPP
