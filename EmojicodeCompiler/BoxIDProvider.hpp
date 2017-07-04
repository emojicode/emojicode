//
//  BoxIDProvider.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 15/04/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef BoxIDProvider_hpp
#define BoxIDProvider_hpp

#include "Type.hpp"
#include <vector>
#include <map>

namespace EmojicodeCompiler {

class BoxIDProvider {
public:
    uint32_t boxIdFor(const std::vector<Type> &genericArguments) {
        auto genericId = genericIds_.find(genericArguments);
        if (genericId != genericIds_.end()) {
            return genericId->second;
        }
        genericIds_.emplace(genericArguments, boxIds);
        return boxIds++;
    }
    static uint32_t maxBoxIndetifier() { return boxIds; }
    const std::map<std::vector<Type>, uint32_t>& genericIds() { return genericIds_; }
private:
    static uint32_t boxIds;
    std::map<std::vector<Type>, uint32_t> genericIds_;
};

};  // namespace EmojicodeCompiler

#endif /* BoxIDProvider_hpp */
