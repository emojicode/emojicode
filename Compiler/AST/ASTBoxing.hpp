//
//  ASTBoxing.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTBoxing_hpp
#define ASTBoxing_hpp

#include "ASTExpr.hpp"
#include "MemoryFlowAnalysis/MFFunctionAnalyser.hpp"
#include "MemoryFlowAnalysis/MFHeapAllocates.hpp"
#include <utility>

namespace EmojicodeCompiler {

class ASTBoxing : public ASTUnaryMFForwarding {
public:
    ASTBoxing(std::shared_ptr<ASTExpr> expr, const SourcePosition &p, const Type &exprType);
    Type analyse(FunctionAnalyser *, const TypeExpectation &) final { return expressionType(); }
    
protected:
    /// Gets a pointer to the value area of box and bit-casts it to the type matching the ASTExpr::expressionType()
    /// of ::expr_.
    /// @param box A pointer to a box.
    Value* getBoxValuePtr(Value *box, FunctionCodeGenerator *fg) const;
    /// Constructs a simple optional struct according to expressionType() and populates it with value.
    Value* getSimpleOptional(Value *value, FunctionCodeGenerator *fg) const;
    /// Constructs a simple optional struct according to expressionType() and populates it with value.
    Value* getSimpleOptionalWithoutValue(FunctionCodeGenerator *fg) const;

    Value* getGetValueFromBox(Value *box, FunctionCodeGenerator *fg) const;
    /// Allocas space for a box and then calls ASTExpr::generate() of of ::expr_ and stores its value into the box.
    Value* getAllocaTheBox(FunctionCodeGenerator *fg) const;

    /// @returns True if the value to be boxed is the result of a value type initialization.
    /// Some boxing operations perform certain optimizations, i.e. avoid copying of the value type, by directly
    /// initializing the value type into the box.
    /// @see valueTypeInit
    bool isValueTypeInit() const { return init_; }
    /// If isValueTypeInit() returns true, this method can be used to initialize the value type to a certain
    /// location. This can be useful for optimizations.
    /// @param destination Pointer to the location at which the value type shall be initialized.
    void valueTypeInit(FunctionCodeGenerator *fg, Value *destination) const;

    Value *getSimpleError(Value *value, FunctionCodeGenerator *fg) const;

private:
    bool init_ = false;
};

class ASTUpcast final : public ASTBoxing {
public:
    ASTUpcast(std::shared_ptr<ASTExpr> expr, const SourcePosition &p, const Type &exprType, const Type &toType) :
        ASTBoxing(expr, p, exprType), toType_(toType) {}

    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(PrettyStream &pretty) const override {}
private:
    Type toType_;
};

class ASTRebox final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(PrettyStream &pretty) const override {}
};

class ASTBoxToSimpleOptional final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(PrettyStream &pretty) const override {}
};

class ASTBoxToSimpleError final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(PrettyStream &pretty) const override {}
};

class ASTSimpleToSimpleOptional final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(PrettyStream &pretty) const override {}
};

class ASTSimpleToSimpleError final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Value *generate(FunctionCodeGenerator *fg) const override;
    void toCode(PrettyStream &pretty) const override {}
};

class ASTToBox : public ASTBoxing, public MFHeapAutoAllocates {
    using ASTBoxing::ASTBoxing;
protected:
    void getPutValueIntoBox(Value *box, Value *value, FunctionCodeGenerator *fg) const;
    void setBoxInfo(Value *box, FunctionCodeGenerator *fg) const;
    Value* buildStoreAddress(Value *box, FunctionCodeGenerator *fg) const;

    void analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) override {
        analyseAllocation(type);
        ASTBoxing::analyseMemoryFlow(analyser, type);
    }
};

class ASTSimpleOptionalToBox final : public ASTToBox {
    using ASTToBox::ASTToBox;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(PrettyStream &pretty) const override {}
};

class ASTSimpleErrorToBox final : public ASTToBox {
    using ASTToBox::ASTToBox;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(PrettyStream &pretty) const override {}
};

class ASTSimpleToBox final : public ASTToBox {
    using ASTToBox::ASTToBox;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(PrettyStream &pretty) const override {}
};

class ASTBoxToSimple final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(PrettyStream &pretty) const override {}
};

class ASTDereference final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(PrettyStream &pretty) const override {}
};

class ASTStoreTemporarily final : public ASTBoxing {
    using ASTBoxing::ASTBoxing;
    Value* generate(FunctionCodeGenerator *fg) const override;
    void toCode(PrettyStream &pretty) const override {}
};

} // namespace EmojicodeCompiler

#endif /* ASTBoxing_hpp */
