//
//  MP3Length.m
//  XNJB
//
//  Created by Richard Low on 01/01/2005.
//

#import "MP3Length.h"
#include "mp3file.h"

@implementation MP3Length

+ (unsigned)length:(NSString *)filename
{
	return length_from_file((char *)[filename fileSystemRepresentation]);
}

@end
