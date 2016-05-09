//
//  TypeDefinitionWithGenerics.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef TypeDefinitionWithGenerics_hpp
#define TypeDefinitionWithGenerics_hpp

#include <vector>
#include "TypeDefinition.hpp"

class TypeDefinitionWithGenerics : public TypeDefinition {
public:
    void addGenericArgument(const Token &variableName, Type constraint);
    void setSuperTypeDef(TypeDefinitionWithGenerics *superTypeDef);
    void setSuperGenericArguments(std::vector<Type> superGenericArguments);
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
    virtual bool canBeUsedToResolve(TypeDefinitionWithGenerics *resolutionConstraint) = 0;
    
    const std::map<EmojicodeString, Type>& ownGenericArgumentVariables() const { return ownGenericArgumentVariables_; }
    const std::vector<Type>& superGenericArguments() const { return superGenericArguments_; }
    const std::vector<Type>& genericArgumentConstraints() const { return genericArgumentConstraints_; }
protected:
    TypeDefinitionWithGenerics(EmojicodeChar name, Package *p, const EmojicodeString &documentation)
        : TypeDefinition(name, p, documentation) {}
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


#endif /* TypeDefinitionWithGenerics_hpp */
