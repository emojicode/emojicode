//
//  Releasing.hpp
//  runtime
//
//  Created by Theo Weidmann on 05.03.19.
//

#ifndef Releasing_hpp
#define Releasing_hpp

#include <vector>
#include <memory>

namespace EmojicodeCompiler {

class ASTRelease;
class FunctionCodeGenerator;

/// Nodes to which release statements can be attached inherit from this class.
///
/// Release statements must be attached to that transfer flow control back to another function (return) as the
/// objects need to be released after the any expressions of the node are evaluated but before the actual return
/// instruction.
class Releasing {
public:
    Releasing();
    
    /// Adds a release statement to be executed before the method ultimately returns but after the return value itself
    /// has been evaluated.
    void addRelease(std::unique_ptr<ASTRelease> release);

    virtual ~Releasing();

protected:
    /// Release all temporary objects and all variables in this scope. To be called before the method ultimately returns
    /// but after the return value has been evaluated.
    void release(FunctionCodeGenerator *fg) const;

private:
    std::vector<std::unique_ptr<ASTRelease>> releases_;
};

}

#endif /* Releasing_hpp */
