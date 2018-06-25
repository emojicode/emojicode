//
//  PrettyPrinter.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 23.06.18.
//

#ifndef PrettyPrinter_hpp
#define PrettyPrinter_hpp

#include "PrettyStream.hpp"
#include "Types/Generic.hpp"
#include "Package/RecordingPackage.hpp"

namespace EmojicodeCompiler {

/// Prettyprinter class generates Emojicode source code or interface code for an RecordingPackage.
class PrettyPrinter {
    friend PrettyStream;
public:
    explicit PrettyPrinter(RecordingPackage *package) : prettyStream_(this), package_(package) {}

    /// Writes an interface file to the provided path.
    /// @param out The path at which the file will be created. If the file exists it is overwritten.
    void printInterface(const std::string &out);
    /// Regenerates the code for the package. All packge files are regenerated in place, the original files are renamed
    /// to preserve them as backup.
    void print();

private:
    PrettyStream prettyStream_;

    RecordingPackage *package_;
    bool interface_ = false;
    size_t interfaceFileIndex = 1;

    void printClosure(Function *function);

    void printRecordings(const std::vector<std::unique_ptr<RecordingPackage::Recording>> &recordings);
    void print(const char *key, Function *function, bool body, bool noMutate);
    void print(RecordingPackage::Recording *recording);
    void printEnumValues(Enum *enumeration);
    void printProtocolConformances(TypeDefinition *typeDef, const TypeContext &typeContext);
    void printInstanceVariables(TypeDefinition *typeDef, const TypeContext &typeContext);
    template<typename T, typename E>
    void printGenericParameters(Generic<T, E> *generic) {
        if (generic->genericParameters().empty()) {
            return;
        }
        prettyStream_.refuseOffer() << "🐚";
        for (auto &param : generic->genericParameters()) {
            if (param.rejectsBoxing) {
                prettyStream_ << "☣️";
            }
            prettyStream_ << utf8(param.name) << " " << param.constraint;
            prettyStream_.offerSpace();
        }
        prettyStream_ << "🍆";
    }
    void printMethodsAndInitializers(TypeDefinition *typeDef);
    void printTypeDefName(const Type &type);
    void printArguments(Function *function);
    void printReturnType(Function *function);
    void printFunctionAttributes(Function *function, bool noMutate);
    void printFunctionAccessLevel(Function *function);
    void printTypeDef(const Type &type);
    void printDocumentation(const std::u32string &doc);
    std::string filePath(const std::string &path);
};

}  // namespace EmojicodeCompiler

#endif /* PrettyPrinter_hpp */
