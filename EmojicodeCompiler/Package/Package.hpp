//
//  Package.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 01/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Package_hpp
#define Package_hpp

#include "../EmojicodeCompiler.hpp"
#include "../Lex/SourcePosition.hpp"
#include "../Types/Extension.hpp"
#include "../Types/Type.hpp"
#include <map>
#include <utility>
#include <vector>

namespace EmojicodeCompiler {

class Function;

#undef major
#undef minor

struct PackageVersion {
    PackageVersion(int major, int minor) : major(major), minor(minor) {}
    /** The major version */
    int major;
    /** The minor version */
    int minor;
};

struct ExportedType {
    ExportedType(Type t, std::u32string n) : type(std::move(t)), name(std::move(n)) {}

    Type type;
    std::u32string name;
};

struct TypeIdentifier;
class Application;
class CompatibilityInfoProvider;

/// Package is the class used to load, parse and analyse packages.
class Package {
public:
    /// Constructs a package. The package must be compiled with compile() before it can be loaded.
    /// @param name The name of the package. The package is registered with this name.
    /// @param mainFilePath The path to the package’s main file. This is the file that is parsed when parse() is called.
    Package(std::string name, std::string mainFilePath, Application *app)
    : name_(std::move(name)), mainFile_(std::move(mainFilePath)), app_(app) {}

    /// Ssemantically analyses and optimizes this package.
    void analyse();
    /// Lexes and parses the package. If this package is the s package the s loading procedure is invoked.
    /// If the package is not the @c s package, the s package is first imported via importPackage().
    void parse();

    /// Tries to import the package identified by name into a namespace of this package.
    /// @see Application::loadPackage
    virtual void importPackage(const std::string &name, const std::u32string &ns, const SourcePosition &p);

    /// Loads, lexes and parses the document at path in the context of this package.
    /// @param path A file path to a source code document.
    /// @param relativePath The path as it was discovered in the source code document. Value is provided for
    ///                     subclasses that might be interested in this. (e.g. RecordingPackage)
    virtual void includeDocument(const std::string &path, const std::string &relativePath);

    /// @returns The application to which this package belongs.
    Application* app() const { return app_; }

    /// @returns The name of this package.
    const std::string& name() const { return name_; }
    /// @returns A SourcePosition that can be used to relate to this package.
    SourcePosition position() const { return SourcePosition(0, 0, mainFile_); }
    /// @returns True iff compile() was called on this package and returned.
    /// If this method returns false and another method tries to load this package, this indicates a circular depedency.
    bool finishedLoading() const { return finishedLoading_; }

    PackageVersion version() const { return version_; }
    /** Whether this package has declared a valid package version. */
    bool validVersion() const { return version().minor > 0 || version().major > 0; }
    void setPackageVersion(PackageVersion v) { version_ = v; }

    /// If the compatibility mode is on the, the parsers will try to parse Emojicode 0.5 syntax.
    /// If this is true, a MigArgs object is provided.
    /// @returns Whether the compatiblity mode is on.
    bool compatibilityMode() { return compatibilityInfoProvider_ != nullptr; }
    CompatibilityInfoProvider* compatibilityInfoProvider() { return compatibilityInfoProvider_; }
    void setCompatiblityInfoProvider(CompatibilityInfoProvider *cac) { compatibilityInfoProvider_ = cac; }

    const std::u32string& documentation() const { return documentation_; }
    void setDocumentation(const std::u32string &doc) { documentation_ = doc; }

    bool requiresBinary() const { return requiresNativeBinary_; }
    void setRequiresBinary(bool b = true) { requiresNativeBinary_ = b; }

    /// A Class defined in this package must be registered with this method.
    void registerClass(Class *cl) { classes_.push_back(cl); }
    /// A ValueType (and its derivates such as Enum) defined in this package must be registered with this method.
    void registerValueType(ValueType *vt) { valueTypes_.push_back(vt); }
    /// Functions that technically belong to this package but are not contained in a Class must be
    /// registered with this function.
    /// @see STIProvider, which is the provider that is used for methods that must be registered with this function.
    void registerFunction(Function *function) { functions_.push_back(function); }
    /// An extension defined in this package that extends a type not defined in this package, must be registered
    /// with this method.
    virtual Extension& registerExtension(Extension ext) {
        extensions_.emplace_back(std::move(ext));
        return extensions_.back();
    }

    /// Make a type available in this package. A type may be provided under different names, so this method may be
    /// called with the same type multiple times with different names and namespaces.
    /// @param t The type to add to the package.
    /// @param name The name under which to make the type available in the namespace.
    /// @param ns The aformentioned namespace.
    /// @param exportFromPkg Whether the type should be exported from the package. Note that the type will be imported
    ///                      in other packages with @c name.
    /// @throws CompilerError If a type with the same name has already been exported.
    virtual void offerType(Type t, const std::u32string &name, const std::u32string &ns, bool exportFromPkg,
                           const SourcePosition &p);

    /// @returns All classes registered with this package.
    const std::vector<Class *>& classes() const { return classes_; };
    const std::vector<Function *>& functions() const { return functions_; }
    const std::vector<ValueType *>& valueTypes() const { return valueTypes_; };
    const std::vector<ExportedType>& exportedTypes() const { return exportedTypes_; };

    /// Tries to fetch a type by its name and namespace from the namespace and types available in this package and
    /// stores it into @c type.
    /// @note This method returns a “raw” type, i.e. a type without generic arguments.
    /// @returns Whether the type could be found or not. @c type is untouched if @c false was returned.
    bool lookupRawType(const TypeIdentifier &typeId, bool optional, Type *type) const;
    /// Like lookupRawType() but throws a CompilerError if the type cannot be found.
    Type getRawType(const TypeIdentifier &typeId, bool optional) const;

    /// @complexity O(n)
    std::u32string findNamespace(const Type &type);
private:
    /// Verifies that no type with name @c name has already been exported and adds the type to ::exportedTypes_
    void exportType(Type t, std::u32string name, const SourcePosition &p);

    void enqueueFunctionsOfTypeDefinition(TypeDefinition *typeDef) const;
    void enqueueFunction(Function *) const;

    const std::string name_;
    const std::string mainFile_;
    PackageVersion version_ = PackageVersion(0, 0);
    bool requiresNativeBinary_ = false;
    bool finishedLoading_ = false;

    CompatibilityInfoProvider *compatibilityInfoProvider_ = nullptr;

    std::map<std::u32string, Type> types_;
    std::vector<ExportedType> exportedTypes_;
    std::vector<Class *> classes_;
    std::vector<ValueType *> valueTypes_;
    std::vector<Function *> functions_;
    std::vector<Extension> extensions_;

    std::u32string documentation_;
    Application *app_;
};

}  // namespace EmojicodeCompiler

#endif /* Package_hpp */
