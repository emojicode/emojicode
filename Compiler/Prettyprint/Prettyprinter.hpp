//
//  Prettyprinter.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 25/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Prettyprinter_hpp
#define Prettyprinter_hpp

#include "../Package/RecordingPackage.hpp"
#include "../Types/TypeContext.hpp"
#include "../Types/Generic.hpp"
#include <fstream>

namespace EmojicodeCompiler {

class Type;
class Function;
class Compiler;
class Package;
class TypeDefinition;

/// Prettyprinter class produces Emojicode source code from an RecordingPackage.
///
/// Prettyprinter can be appended to  with <<. Prettyprinter then appends data as necessary to the underlying stream.
///
/// Prettyprinter features a concept of "whitespace offers". By a call to offerSpace() or offerNewLine() at whitespace
/// is offered. The whitespace is then appended before the next object passed to << if refuseOffer() isn’t called
/// before.
class Prettyprinter {
public:
    Prettyprinter(RecordingPackage *package) : package_(package) {}
    void printInterface(const std::string &out);
    void print();

    /// Appends the whitespace offer to the stream if any is available. Then appends rhs and returns this instance.
    template<typename T>
    Prettyprinter& operator<<(const T &rhs) {
        if (whitespaceOffer_ != 0) {
            stream_ << whitespaceOffer_;
            whitespaceOffer_ = 0;
        }
        stream_ << rhs;
        return *this;
    }

    Prettyprinter& operator<<(const Type &type) {
        print(type, typeContext_);
        return *this;
    }

    Prettyprinter& thisStream() { return *this; }

    void printClosure(Function *function);

    /// Appends the requested amount of indentation characters to the stream and returns it.
    Prettyprinter& indent() { return *this << std::string(indentation_ * 2, ' '); }

    void increaseIndent() { indentation_++; }
    void decreaseIndent() { indentation_--; }

    /// Refuses any available whitespace offer.
    /// @returns The instance.
    Prettyprinter& refuseOffer() { whitespaceOffer_ = 0; return *this; }
    /// Offers a space character
    void offerSpace() { whitespaceOffer_ = ' '; }
    /// Offers a new line character
    void offerNewLine() { whitespaceOffer_ = '\n'; }
    /// Calls offerSpace() unless collection returns true for empty()
    template<typename T>
    void offerNewLineUnlessEmpty(const T &collection) { if (!collection.empty()) { offerNewLine(); } }
private:
    std::fstream stream_;
    RecordingPackage *package_;
    TypeContext typeContext_;
    char whitespaceOffer_ = 0;
    unsigned int indentation_ = 0;
    bool interface_ = false;

    void print(const char *key, Function *function, bool body, bool noMutate);
    void print(RecordingPackage::Recording *recording);
    void printNamespaceAccessor(const Type &type);
    void printEnumValues(Enum *enumeration);
    void printProtocolConformances(TypeDefinition *typeDef, const TypeContext &typeContext);
    void printInstanceVariables(TypeDefinition *typeDef, const TypeContext &typeContext);
    template<typename T>
    void printGenericParameters(Generic<T> *generic) {
        for (auto &param : generic->parameters()) {
            refuseOffer() << "🐚" << utf8(param.first) << " " << param.second;
            offerSpace();
        }
    }
    void printMethodsAndInitializers(TypeDefinition *typeDef);
    void printTypeDefName(const Type &type);
    void print(const Type &type, const TypeContext &typeContext);
    void printArguments(Function *function);
    void printReturnType(Function *function);
    void printFunctionAttributes(Function *function, bool noMutate);
    void printFunctionAccessLevel(Function *function);
    void printTypeDef(const Type &type);
    void printDocumentation(const std::u32string &doc);
    std::string filePath(const std::string &path);
};

}  // namespace EmojicodeCompiler


#endif /* Prettyprinter_hpp */
