//
//  TypeDefinitionFunctional.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef TypeDefinitionFunctional_hpp
#define TypeDefinitionFunctional_hpp

#include <vector>
#include "TypeDefinition.hpp"
#include "Type.hpp"

class TypeContext;
class Initializer;
class Function;

class TypeDefinitionFunctional : public TypeDefinition {
public:
    /**
     * Adds a new generic argument to the end of the list. 
     * @param variableName The name which is used to refer to this argument.
     * @param constraint The constraint that applies to the types passed.
     */
    void addGenericArgument(const Token &variableName, Type constraint);
    void setSuperTypeDef(TypeDefinitionFunctional *superTypeDef);
    void setSuperGenericArguments(std::vector<Type> superGenericArguments);
    /** Must be called before the type is used but after the last generic argument was added. */
    void finalizeGenericArguments();
    
    /** Returns the number of generic arguments this type takes when referenced to in Emojicode source code. */
    uint16_t numberOfOwnGenericArguments() const { return ownGenericArgumentCount_; }
    /**
     * Returns the number of generic arguments a type of this type definition stores when initialized.
     * This therefore also includes all arguments to supertypedefinitions of this type.
     */
    uint16_t numberOfGenericArgumentsWithSuperArguments() const { return genericArgumentCount_; }
    /**
     * Tries to fetch the type reference type for the given generic variable name and stores it into @c type.
     * @returns Whether the variable could be found or not. @c type is untouched if @c false was returned.
     */
    bool fetchVariable(EmojicodeString name, bool optional, Type *type);
    /*
     * Determines whether the given type reference resolution constraint allows the type to be
     * resolved on this type definition.
     */
    virtual bool canBeUsedToResolve(TypeDefinitionFunctional *resolutionConstraint) = 0;
    
    const std::map<EmojicodeString, Type>& ownGenericArgumentVariables() const { return ownGenericArgumentVariables_; }
    const std::vector<Type>& superGenericArguments() const { return superGenericArguments_; }
    const std::vector<Type>& genericArgumentConstraints() const { return genericArgumentConstraints_; }
    
    /** Returns a method by the given identifier token or issues an error if the method does not exist. */
    Function* getMethod(const Token &token, Type type, TypeContext typeContext);
    /** Returns an initializer by the given identifier token or issues an error if the initializer does not exist. */
    Initializer* getInitializer(const Token &token, Type type, TypeContext typeContext);
    /** Returns a method by the given identifier token or issues an error if the method does not exist. */
    Function* getClassMethod(const Token &token, Type type, TypeContext typeContext);
    
    /** Returns a method by the given identifier token or @c nullptr if the method does not exist. */
    virtual Function* lookupMethod(EmojicodeChar name);
    /** Returns a initializer by the given identifier token or @c nullptr if the initializer does not exist. */
    virtual Initializer* lookupInitializer(EmojicodeChar name);
    /** Returns a method by the given identifier token or @c nullptr if the method does not exist. */
    virtual Function* lookupClassMethod(EmojicodeChar name);
    
    virtual void addMethod(Function *method);
    virtual void addInitializer(Initializer *method);
    virtual void addClassMethod(Function *method);
    
    const std::vector<Function *>& methodList() const { return methodList_; }
    const std::vector<Initializer *>& initializerList() const { return initializerList_; }
    const std::vector<Function *>& classMethodList() const { return classMethodList_; }
protected:
    TypeDefinitionFunctional(EmojicodeChar name, Package *p, SourcePosition pos, const EmojicodeString &documentation)
        : TypeDefinition(name, p, pos, documentation) {}
    
    std::map<EmojicodeChar, Function *> methods_;
    std::map<EmojicodeChar, Function *> classMethods_;
    std::map<EmojicodeChar, Initializer *> initializers_;
    
    std::vector<Function *> methodList_;
    std::vector<Initializer *> initializerList_;
    std::vector<Function *> classMethodList_;
    
    virtual void handleRequiredInitializer(Initializer *init);
private:
    /** The number of generic arguments including those from a super eclass. */
    uint16_t genericArgumentCount_ = 0;
    /** The number of generic arguments this eclass takes. */
    uint16_t ownGenericArgumentCount_ = 0;
    /** The types for the generic arguments. */
    std::vector<Type> genericArgumentConstraints_;
    /** The arguments for the classes from which this eclass inherits. */
    std::vector<Type> superGenericArguments_;
    /** Generic type arguments as variables */
    std::map<EmojicodeString, Type> ownGenericArgumentVariables_;
};


#endif /* TypeDefinitionFunctional_hpp */
