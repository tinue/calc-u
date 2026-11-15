#include "MacAppSupport.h"

#import <Foundation/Foundation.h>

void macDisableWindowRestoration() {
    @autoreleasepool {
        [[NSUserDefaults standardUserDefaults] registerDefaults:@{@"ApplePersistenceIgnoreState" : @YES}];
    }
}

void macDisablePressAndHold() {
    @autoreleasepool {
        [[NSUserDefaults standardUserDefaults] registerDefaults:@{@"ApplePressAndHoldEnabled" : @NO}];
    }
}

std::string macApplicationSupportFolder() {
    @autoreleasepool {
        NSURL* url = [[NSFileManager defaultManager] URLForDirectory:NSApplicationSupportDirectory
                                                            inDomain:NSUserDomainMask
                                                   appropriateForURL:nil
                                                              create:NO
                                                               error:nil];
        if (url == nil || url.fileSystemRepresentation == nullptr) return {};
        return std::string(url.fileSystemRepresentation);
    }
}
