//
//  Package.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 01/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Package_hpp
#define Package_hpp

#include <list>
#include <array>
#include <map>
#include "EmojicodeCompiler.hpp"
#include "Type.hpp"
#include "Token.hpp"

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
    ExportedType(Type t, EmojicodeString n) : type(t), name(n) {}
    
    Type type;
    EmojicodeString name;
};

struct ParsedTypeName;

class Package {
public:
    /** 
     * Tries to load a package identified by its name into a namespace in this package.
     * This method tries to find the package to load in the cache first and prevents
     * circular dependencies.
     * @param name The name of the package to load.
     */
    Package* loadPackage(std::string name, EmojicodeString ns, SourcePosition p);
    
    Package(std::string n, SourcePosition p) : name_(n), position_(p) {}
    void parse(const std::string path);
    
    bool finishedLoading() const { return finishedLoading_; }
    PackageVersion version() const { return version_; }
    /** Whether this package has declared a valid package version. */
    bool validVersion() const { return version().minor > 0 || version().major > 0; }
    void setPackageVersion(PackageVersion v) { version_ = v; }
    bool requiresBinary() const { return requiresNativeBinary_; }
    void setRequiresBinary(bool b = true) { requiresNativeBinary_ = b; }
    const std::string& name() const { return name_; }
    
    const SourcePosition& position() const { return position_; }
    
    void exportType(Type t, EmojicodeString name);
    void registerClass(Class *cl) { classes_.push_back(cl); }
    void registerType(Type t, EmojicodeString name, EmojicodeString ns, bool exportFromPackage);
    
    const std::list<Class *>& classes() const { return classes_; };
    const std::list<ExportedType>& exportedTypes() const { return exportedTypes_; };

    /**
     * Tries to fetch a type by its name and namespace and stores it into @c type.
     * @return Whether the type could be found or not. @c type is untouched if @c false was returned.
     */
    bool fetchRawType(EmojicodeString name, EmojicodeString ns, bool opt, SourcePosition errorPosition, Type *type);
    bool fetchRawType(ParsedTypeName ptn, Type *type);

    /** Returns the loaded packages in the order in which they were loaded. */
    static const std::list<Package *>& packagesInOrder() { return packagesLoadingOrder_; };
    /** Searches the loaded packages for the package with the given name. If the package has not been loaded @c nullptr
        is returned. */
    static Package* findPackage(const std::string &name);
    
    const EmojicodeString& documentation() const { return documentation_; }
    void setDocumentation(const EmojicodeString &doc) { documentation_ = doc; }
private:
    void loadInto(Package *destinationPackage, EmojicodeString ns, const Token &errorToken) const;
    
    std::string name_;
    PackageVersion version_ = PackageVersion(0, 0);
    bool requiresNativeBinary_ = false;
    bool finishedLoading_ = false;
    SourcePosition position_;
    
    std::map<EmojicodeString, Type> types_;
    std::list<ExportedType> exportedTypes_;
    std::list<Class *> classes_;
    
    static std::list<Package *> packagesLoadingOrder_;
    static std::map<std::string, Package *> packages_;
    
    EmojicodeString documentation_;
};

#endif /* Package_hpp */
