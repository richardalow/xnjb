//
//  MyItem.h
//  XNJB
//
//  Created by Richard Low on 07/09/2004.
//

#import <Cocoa/Cocoa.h>
#include "libmtp.h"

@interface MyItem : NSObject {
	unsigned itemID;
	LIBMTP_filetype_t fileType;
}
- (BOOL)matchesString:(NSString *)search;
- (void)setItemID:(unsigned)newItemID;
- (unsigned)itemID;
- (LIBMTP_filetype_t)fileType;
- (void)setFileType:(LIBMTP_filetype_t)newFileType;
- (NSString *)fileTypeString;
- (void)fileTypeFromExtension:(NSString *)filename;

@end
