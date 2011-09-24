//
//  FileSizeFormatter.h
//  XNJB
//
//  Created by Richard Low on Sat Aug 21 2004.
//

#import <Foundation/Foundation.h>

@interface FilesizeFormatter : NSObject {

}
+ (NSString *)filesizeString:(unsigned long long)filesize;

@end
