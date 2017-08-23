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
#include <array>
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
    ExportedType(Type t, EmojicodeString n) : type(std::move(t)), name(std::move(n)) {}

    Type type;
    EmojicodeString name;
};

struct TypeIdentifier;

/// Package is the class used to load, parse and analyse packages.
/// This class keeps track of all loaded packages. Packages can be queried with findPackage().
class Package {
public:
    /**
     * Tries to load a package identified by its name into a namespace in this package.
     * This method tries to find the package to load in the cache first and prevents
     * circular dependencies.
     * @param name The name of the package to load.
     */
    Package* loadPackage(const std::string &name, const EmojicodeString &ns, const SourcePosition &p);
    /** Returns the loaded packages in the order in which they were loaded. */
    static const std::vector<Package *>& packagesInOrder() { return packagesLoadingOrder_; };
    /** Searches the loaded packages for the package with the given name. If the package has not been loaded @c nullptr
     is returned. */
    static Package* findPackage(const std::string &name);

    /// Constructs a package. The package must be compiled with compile() before it can be loaded.
    /// @param name The name of the package. The package is registered with this name.
    /// @param mainFilePath The path to the package’s main file.
    Package(std::string name, std::string mainFilePath) : name_(std::move(name)), mainFile_(std::move(mainFilePath)) {}

    /// Lexes, parser, semantically analyses and optimizes this package.
    /// If the package is not the @c s package, the s package is loaded first.
    void compile();

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

    const EmojicodeString& documentation() const { return documentation_; }
    void setDocumentation(const EmojicodeString &doc) { documentation_ = doc; }

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
    void registerExtension(Extension ext) { extensions_.emplace_back(std::move(ext)); }

    /// Make a type available in this package. A type may be provided under different names, so this method may be
    /// called with the same type multiple times with different names and namespaces.
    /// @param t The type to add to the package.
    /// @param name The name under which to make the type available in the namespace.
    /// @param ns The aformentioned namespace.
    /// @param exportFromPkg Whether the type should be exported from the package. Note that the type will be imported
    ///                      in other packages with @c name.
    /// @throws CompilerError If a type with the same name has already been exported.
    void offerType(Type t, const EmojicodeString &name, const EmojicodeString &ns, bool exportFromPkg,
                   const SourcePosition &p);

    /// @returns All classes registered with this package.
    const std::vector<Class *>& classes() const { return classes_; };
    const std::vector<Function *>& functions() const { return functions_; }
    const std::vector<ExportedType>& exportedTypes() const { return exportedTypes_; };

    /// Tries to fetch a type by its name and namespace from the namespace and types available in this package and
    /// stores it into @c type.
    /// @returns Whether the type could be found or not. @c type is untouched if @c false was returned.
    bool fetchRawType(const EmojicodeString &name, const EmojicodeString &ns, bool optional, const SourcePosition &p,
                      Type *type);
    /// Calls fetchRawType() with the contents of the TypeIdentifier
    bool fetchRawType(TypeIdentifier ptn, bool optional, Type *type);
private:
    void loadInto(Package *destinationPackage, const EmojicodeString &ns, const SourcePosition &p) const;
    /// Verifies that no type with name @c name has already been exported and adds the type to ::exportedTypes_
    void exportType(Type t, EmojicodeString name, const SourcePosition &p);

    void parse();
    void analyse();

    void enqueueFunctionsOfTypeDefinition(TypeDefinition *typeDef);
    void enqueFunction(Function *);

    const std::string name_;
    const std::string mainFile_;
    PackageVersion version_ = PackageVersion(0, 0);
    bool requiresNativeBinary_ = false;
    bool finishedLoading_ = false;

    std::map<EmojicodeString, Type> types_;
    std::vector<ExportedType> exportedTypes_;
    std::vector<Class *> classes_;
    std::vector<ValueType *> valueTypes_;
    std::vector<Function *> functions_;
    std::vector<Extension> extensions_;

    static std::vector<Package *> packagesLoadingOrder_;
    static std::map<std::string, Package *> packages_;

    EmojicodeString documentation_;
};

}  // namespace EmojicodeCompiler

#endif /* Package_hpp */
