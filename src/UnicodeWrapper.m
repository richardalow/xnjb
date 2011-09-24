//
//  UnicodeWrapper.m
//  XNJB
//
//  Created by Richard Low on 17/09/2004.
//

#import "UnicodeWrapper.h"
#include "unicode.h"

@implementation UnicodeWrapper

+ (NSString *)stringFromUTF16:(unsigned short *)uni
{
	char *utf8 = g_utf16_to_utf8(uni, -1, NULL, NULL);
	if (utf8 == NULL)
		return nil;
	NSString *stringUTF8 = [NSString stringWithUTF8String:utf8];
	free(utf8);
	return stringUTF8;
}

// the return must be freed using free()
// as of version 1.1.4 we don't do any endian swapping-just return the short as we should
+ (unsigned short *)UTF16FromString:(NSString *)str
{
	const char *uni = [str UTF8String];
	unsigned short *uniShort = g_utf8_to_utf16(uni, -1, NULL, NULL);
		
	return uniShort;
}

@end
