//
//  VTIProvider.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 25/10/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef VTIProvider_hpp
#define VTIProvider_hpp

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

class ClassVTIProvider : public VTIProvider {
public:
    /** Returns a new VTI for a function. This method is called by @c Function’s @c assignVTI */
    virtual int next() override { vtiCount_++; return nextVti_++; }
    /** Returns the number of VTIs that were used. This counter is incremented by either calling @c next() or
        @c incrementVtiCount() */
    int vtiCount() const { return vtiCount_; }
    /** Increments the counter whose value is returned from @c vtiCount */
    void incrementVtiCount() { vtiCount_++; }
    /** Sets the next VTI to be assigned to @c offset. */
    void offsetVti(int offset) { nextVti_ = offset; }
    /** Returns the VTI that would be next without actually consuming it. */
    int peekNext() const { return nextVti_; }
private:
    int nextVti_ = 0;
    int vtiCount_ = 0;
};

class ValueTypeVTIProvider : public VTIProvider {
public:
    virtual int next() override;
    int vtiCount() const { return vtiCount_; }
private:
    int vtiCount_ = 0;
};

#endif /* VTIProvider_hpp */
