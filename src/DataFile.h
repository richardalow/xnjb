//
//  DataFile.h
//  XNJB
//
//  Created by Richard Low on Sat Aug 21 2004.
//

#import <Foundation/Foundation.h>
#import "MyItem.h"

@interface DataFile : MyItem {
  unsigned long long size;
	NSString *filename;
//	NSDate *timestamp;
	NSString *fullPath;
}
- (void)setSize:(unsigned long long)newSize;
- (unsigned long long)size;
- (void)setFilename:(NSString *)newFilename;
- (NSString *)filename;
//- (void)setTimestamp:(NSDate *)newTimestamp;
//- (void)setTimestampSince1970:(unsigned int)newTimestamp;
//- (NSDate *)timestamp;
- (void)setFullPath:(NSString *)newFullPath;
- (NSString *)fullPath;
- (NSString *)sizeString;
//- (NSString *)timestampString;
+ (DataFile *)dataFileFromPath:(NSString *)path;
- (void)fileTypeFromExtension;

@end
