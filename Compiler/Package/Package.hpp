//
//  Package.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 01/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Package_hpp
#define Package_hpp

#include "Types/Type.hpp"
#include <map>
#include <utility>
#include <vector>
#include <memory>

namespace EmojicodeCompiler {

class Function;
class TokenStream;

struct ExportedType {
    ExportedType(Type t, std::u32string n) : type(std::move(t)), name(std::move(n)) {}

    Type type;
    std::u32string name;
};

struct TypeIdentifier;
struct SourcePosition;
class Compiler;

/// Package is the class used to load, parse and analyse packages.
class Package {
public:
    /// Constructs a package. The package must be compiled with compile() before it can be loaded.
    /// @param name The name of the package. The package is registered with this name.
    /// @param path The path to the directory of the package.
    Package(std::string name, std::string path, Compiler *app, bool imported);

    /// Lexes and parses the package.
    /// If this package is the s package the s loading procedure is invoked.
    /// If the package is not the @c s package, the s package is first imported via importPackage().
    void parse(const std::string &mainFilePath);

    /// Tries to import the package identified by name into a namespace of this package.
    /// @see Compiler::loadPackage
    virtual void importPackage(const std::string &name, const std::u32string &ns, const SourcePosition &p);

    /// Loads, lexes and parses the document at path in the context of this package.
    /// @param path A file path to a source code document.
    /// @param relativePath The path as it was discovered in the source code document. Value is provided for
    ///                     subclasses that might be interested in this. (e.g. RecordingPackage)
    virtual void includeDocument(const std::string &path, const std::string &relativePath);

    /// @returns The application to which this package belongs.
    Compiler* compiler() const { return compiler_; }

    /// @returns The name of this package.
    const std::string& name() const { return name_; }

    /// @returns True iff compile() was called on this package and returned.
    /// If this method returns false and another method tries to load this package, this indicates a circular depedency.
    bool finishedLoading() const { return finishedLoading_; }

    bool isImported() const { return imported_; }

    /// @returns The path to the directory of this package.
    const std::string& path() const { return path_; }

    const std::u32string& documentation() const { return documentation_; }
    void setDocumentation(const std::u32string &doc) { documentation_ = doc; }

    void setStartFlagFunction(Function *function) { startFlag_ = function; }
    Function* startFlagFunction() const { return startFlag_; }
    /// Whether a start flag function was encountered and compiled.
    bool hasStartFlagFunction() const { return startFlag_ != nullptr; }

    /// A Class defined in this package must be registered with this method.
    Class* add(std::unique_ptr<Class> &&cl);
    /// A ValueType (and its derivates such as Enum) defined in this package must be registered with this method.
    ValueType* add(std::unique_ptr<ValueType> &&vt);
    /// A Protocol defined in this package must be registered with this method.
    Protocol* add(std::unique_ptr<Protocol> &&protocol);
    /// Functions that technically belong to this package but are not members of a TypeDefinition instance that is
    /// registered with this package, must be registered with this function.
    Function* add(std::unique_ptr<Function> &&function);

    /// Make a type available in this package. A type may be provided under different names, so this method may be
    /// called with the same type multiple times with different names and namespaces.
    /// @param t The type to add to the package.
    /// @param name The name under which to make the type available in the namespace.
    /// @param ns The aforementioned namespace.
    /// @param exportFromPkg Whether the type should be exported from the package. Note that the type will be imported
    ///                      in other packages with @c name.
    /// @throws CompilerError If a type with the same name has already been exported.
    virtual void offerType(Type t, const std::u32string &name, const std::u32string &ns, bool exportFromPkg,
                           const SourcePosition &p);

    void recreateClassTypes();

    /// @returns All classes registered with this package.
    const std::vector<std::unique_ptr<Class>>& classes() const { return classes_; };
    const std::vector<std::unique_ptr<Function>>& functions() const { return functions_; }
    const std::vector<std::unique_ptr<ValueType>>& valueTypes() const { return valueTypes_; }
    const std::vector<std::unique_ptr<Protocol>>& protocols() const { return protocols_; }
    const std::vector<ExportedType>& exportedTypes() const { return exportedTypes_; }

    /// Tries to fetch a type by its name and namespace from the namespace and types available in this package and
    /// stores it into @c type.
    /// @note This method returns a “raw” type, i.e. a type without generic arguments.
    /// @returns Whether the type could be found or not. @c type is untouched if @c false was returned.
    bool lookupRawType(const TypeIdentifier &typeId, Type *type) const;
    /// Like lookupRawType() but throws a CompilerError if the type cannot be found.
    Type getRawType(const TypeIdentifier &typeId) const;

    /// @complexity O(n)
    std::u32string findNamespace(const Type &type);

    void setLinkHints(std::vector<std::string> hints) { linkHints_ = std::move(hints); }
    const std::vector<std::string>& linkHints() { return linkHints_; }

    virtual ~Package();
    
private:
    /// Verifies that no type with name @c name has already been exported and adds the type to ::exportedTypes_
    void exportType(Type t, std::u32string name, const SourcePosition &p);

    TokenStream lexFile(const std::string &path);

    const std::string name_;
    const std::string path_;
    bool finishedLoading_ = false;
    Function *startFlag_ = nullptr;
    bool imported_;

    std::map<std::u32string, Type> types_;
    std::vector<ExportedType> exportedTypes_;
    std::vector<std::unique_ptr<Class>> classes_;
    std::vector<std::unique_ptr<ValueType>> valueTypes_;
    std::vector<std::unique_ptr<Function>> functions_;
    std::vector<std::unique_ptr<Protocol>> protocols_;
    std::vector<std::string> linkHints_;

    std::u32string documentation_;
    Compiler *compiler_;
};

}  // namespace EmojicodeCompiler

#endif /* Package_hpp */
