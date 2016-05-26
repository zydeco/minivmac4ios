//
//  compat.m
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 26/05/2016.
//  Copyright © 2016 namedfork. All rights reserved.
//

@import Foundation;
@import UIKit;
@import ObjectiveC.runtime;

@implementation NSObject (Compat)

/// Add newSelector if it doesn't exist, backed by originalSelector
+ (void)_nfCompatAddInstanceMethod:(SEL)newSelector withCompatibilityMethod:(SEL)originalSelector {
    if (![self instancesRespondToSelector:newSelector]) {
        Method m = class_getInstanceMethod(self, originalSelector);
        class_addMethod(self, newSelector, method_getImplementation(m), method_getTypeEncoding(m));
    }
}

+ (void)_nfCompatAddClassMethod:(SEL)newSelector withCompatibilityMethod:(SEL)originalSelector {
    if (class_getClassMethod(self, newSelector) == NULL) {
        Method m = class_getClassMethod(self, originalSelector);
        class_addMethod(object_getClass(self), newSelector, method_getImplementation(m), method_getTypeEncoding(m));
    }
}

@end

@implementation NSString (Compat)

+ (void)load {
    [NSString _nfCompatAddInstanceMethod:@selector(containsString:) withCompatibilityMethod:@selector(_nfCompatContainsString:)];
}

- (BOOL)_nfCompatContainsString:(NSString*)string {
    return [self rangeOfString:string].location != NSNotFound;
}

@end

@implementation UIFont (Compat)

+ (void)load {
    [UIFont _nfCompatAddClassMethod:@selector(systemFontOfSize:weight:) withCompatibilityMethod:@selector(_nfCompatSystemFontOfSize:weight:)];
}

+ (UIFont *)_nfCompatSystemFontOfSize:(CGFloat)fontSize weight:(CGFloat)weight {
    return [self systemFontOfSize:fontSize];
}

@end