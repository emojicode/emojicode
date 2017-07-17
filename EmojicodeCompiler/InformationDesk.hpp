//
//  InformationDesk.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 17/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef InformationDesk_hpp
#define InformationDesk_hpp

#include "AbstractParser.hpp"
#include <string>

namespace EmojicodeCompiler {
    
    class InformationDesk : public AbstractParser {
    public:
        InformationDesk(Package *_pkg) : AbstractParser(_pkg, TokenStream()) {}
        void sizeOfVariable(const std::string &string);
    private:
        void inform();
    };

}  // namespace EmojicodeCompiler

#endif /* InformationDesk_hpp */
