//
//  PackageReporter.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef PackageReporter_hpp
#define PackageReporter_hpp

namespace EmojicodeCompiler {

class Package;

namespace CLI {

void reportPackage(Package *package);

}  // namespace CLI

}  // namespace EmojicodeCompiler

#endif /* PackageReporter_hpp */
