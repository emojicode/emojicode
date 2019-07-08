//
//  PackageCreator.hpp
//  runtime
//
//  Created by Theo Weidmann on 30.03.19.
//

#ifndef Creator_hpp
#define Creator_hpp

namespace EmojicodeCompiler {

class Protocol;
class Package;
class ValueType;
class Class;
class CodeGenerator;
class Type;
class Function;

/// A PackageCreator is responsible for declaring and defining all types, functions and run-time information (this
/// includes dispatch and conformance tables) as necessary.
class PackageCreator {
public:
    PackageCreator(Package *package, CodeGenerator *cg) : package_(package), generator_(cg) {}

    void generate();

protected:
    Package *package_;
    CodeGenerator *generator_;

    virtual void createClassInfo(Class *klass);
    virtual void createProtocolTables(const Type &type);
    virtual void createBoxInfo(ValueType *valueType);
    virtual void createFunction(Function *function);
    virtual void createDestructor(Class *klass);
    virtual void createDestructorRetain(ValueType *valueType);

private:
    void createProtocol(Protocol *protocol);
    void createValueType(ValueType *valueType);
    void createClass(Class *klass);
};

class ImportedPackageCreator : public PackageCreator {
    using PackageCreator::PackageCreator;

protected:
    void createClassInfo(Class *klass) override;
    void createProtocolTables(const Type &type) override;
    void createBoxInfo(ValueType *valueType) override;
    void createFunction(Function *function) override;
    void createDestructor(Class *klass) override;
    void createDestructorRetain(ValueType *valueType) override;
};

}

#endif /* Creator_hpp */
