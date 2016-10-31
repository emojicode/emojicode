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
    PackageVersion(uint16_t major, uint16_t minor) : major(major), minor(minor) {}
    /** The major version */
    uint16_t major;
    /** The minor version */
    uint16_t minor;
};

struct ExportedType {
    ExportedType(Type t, EmojicodeChar n) : type(t), name(n) {}
    
    Type type;
    EmojicodeChar name;
};

class Package {
public:
    /** 
     * Tries to load a package identified by its name into a namespace in this package.
     * This method tries to find the package to load in the cache first and prevents
     * circular dependencies.
     * @param name The name of the package to load.
     */
    Package* loadPackage(const char *name, EmojicodeChar ns, SourcePosition p);
    
    Package(const char *n, SourcePosition p) : name_(n), position_(p) {}
    void parse(const char *path);
    
    bool finishedLoading() const { return finishedLoading_; }
    PackageVersion version() const { return version_; }
    /** Whether this package has declared a valid package version. */
    bool validVersion() const { return version().minor > 0 || version().major > 0; }
    void setPackageVersion(PackageVersion v) { version_ = v; }
    bool requiresBinary() const { return requiresNativeBinary_; }
    void setRequiresBinary(bool b = true) { requiresNativeBinary_ = b; }
    const char* name() const { return name_; }
    
    const SourcePosition& position() const { return position_; }
    
    void exportType(Type t, EmojicodeChar name);
    void registerClass(Class *cl) { classes_.push_back(cl); }
    void registerType(Type t, EmojicodeChar name, EmojicodeChar ns, bool exportFromPackage);
    
    const std::list<Class *>& classes() const { return classes_; };
    const std::list<ExportedType>& exportedTypes() const { return exportedTypes_; };
    
    /**
     * Tries to fetch a type by its name and namespace and stores it into @c type.
     * @return Whether the type could be found or not. @c type is untouched if @c false was returned.
     */
    bool fetchRawType(EmojicodeChar name, EmojicodeChar ns, bool optional, SourcePosition errorPosition, Type *type);
    /** Returns the loaded packages in the order in which they were loaded. */
    static const std::list<Package *>& packagesInOrder() { return packagesLoadingOrder_; };
    /** Searches the loaded packages for the package with the given name. If the package has not been loaded @c nullptr
        is returned. */
    static Package* findPackage(const char *name);
    
    const EmojicodeString& documentation() const { return documentation_; }
    void setDocumentation(const EmojicodeString &doc) { documentation_ = doc; }
private:
    void loadInto(Package *destinationPackage, EmojicodeChar ns, const Token &errorToken) const;
    
    const char *name_;
    PackageVersion version_ = PackageVersion(0, 0);
    bool requiresNativeBinary_ = false;
    bool finishedLoading_ = false;
    SourcePosition position_;
    
    std::map<std::array<EmojicodeChar, 2>, Type> types_;
    std::list<ExportedType> exportedTypes_;
    std::list<Class *> classes_;
    
    static std::list<Package *> packagesLoadingOrder_;
    static std::map<std::string, Package *> packages_;
    
    EmojicodeString documentation_;
};

#endif /* Package_hpp */
