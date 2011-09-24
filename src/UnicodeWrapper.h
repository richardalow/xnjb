//
//  UnicodeWrapper.h
//  XNJB
//
//  Created by Richard Low on 17/09/2004.
//

#import <Cocoa/Cocoa.h>

@interface UnicodeWrapper : NSObject {

}
+ (NSString *)stringFromUTF16:(unsigned short *)uni; 
+ (unsigned short *)UTF16FromString:(NSString *)str;

@end
