//
//  VTIProvider.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 25/10/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef VTIProvider_hpp
#define VTIProvider_hpp

namespace EmojicodeCompiler {

/** The VTIProvider is repsonsible for managing the VTI a TypeDefinition assigns to its function
    members. */
class VTIProvider {
public:
    virtual int next() = 0;
    void used() { usedCount_++; }
    int usedCount() const { return usedCount_; }
private:
    int usedCount_ = 0;
};

class ClassVTIProvider final : public VTIProvider {
public:
    /** Returns a new VTI for a function. This method is called by @c Function’s @c assignVTI */
    int next() override { return nextVti_++; }
    /** Sets the next VTI to be assigned to @c offset. */
    void offsetVti(int offset) { nextVti_ = offset; }
    /** Returns the VTI that would be next without actually consuming it. */
    int peekNext() const { return nextVti_; }
private:
    int nextVti_ = 0;
};
    
}  // namespace EmojicodeCompiler

#endif /* VTIProvider_hpp */
