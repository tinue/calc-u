#include "MacAppSupport.h"

#import <Foundation/Foundation.h>

void macKeepTimersPrecise() {
    @autoreleasepool {
        // Held for the rest of the process: App Nap would otherwise stretch
        // the frame timer of an app whose window is not in front.
        static id<NSObject> activity = nil;
        if (activity) return;
        activity = [[NSProcessInfo processInfo]
            beginActivityWithOptions:NSActivityUserInitiatedAllowingIdleSystemSleep | NSActivityLatencyCritical
                              reason:@"PC-1500/PC-1600 emulation keeps real-time pace in the background"];
#if !__has_feature(objc_arc)
        [activity retain];   // without ARC the activity would end when the pool drains
#endif
    }
}

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
