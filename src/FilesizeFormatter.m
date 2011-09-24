//
//  FileSizeFormatter.m
//  XNJB
//
//  Created by Richard Low on Sat Aug 21 2004.
//

/* formats a filesize into a human readable
 * format e.g. 1.3 MB, 412 bytes
 */

#import "defs.h"
#import "FilesizeFormatter.h"

#define KB 1024
#define MB 1048576
#define GB 1073741824

// declare the private methods
@interface FilesizeFormatter (PrivateAPI)
+ (NSString *)round:(double)number;
@end

@implementation FilesizeFormatter

+ (NSString *)filesizeString:(unsigned long long)filesize
{
	NSString *filesizeString;
	if (filesize < KB)
		filesizeString = [NSString stringWithFormat:@"%qu %@", filesize, NSLocalizedString(@"bytes", nil)];
	else if (filesize < MB)
		filesizeString = [NSString stringWithFormat:@"%@ %@", [self round:((double)filesize / KB)], NSLocalizedString(@"KB", nil)];
	else if (filesize < GB)
		filesizeString = [NSString stringWithFormat:@"%@ %@", [self round:((double)filesize / MB)], NSLocalizedString(@"MB", nil)];
	else
		filesizeString = [NSString stringWithFormat:@"%@ %@", [self round:((double)filesize / GB)], NSLocalizedString(@"GB", nil)];
	return filesizeString;
}

+ (NSString *)round:(double)number
{
	int intPart = (int)(number);
	if (intPart >= 10)
		return [NSString stringWithFormat:@"%d", intPart];
	
	double fracPartTimes10 = 10.0*(number - (double)intPart);
	int secondDigit = (int)fracPartTimes10;
	int thirdDigit = (int)((fracPartTimes10 - (double)secondDigit) * 10.0);
	if (thirdDigit >= 5)
	{
		if (secondDigit == 9)
			if (intPart == 9)
				return @"10";
			else
				return [NSString stringWithFormat:@"%d.0", intPart+1];
		else
			return [NSString stringWithFormat:@"%d.%d", intPart, secondDigit+1];
	}
	else
		return [NSString stringWithFormat:@"%d.%d", intPart, secondDigit];
}

@end
