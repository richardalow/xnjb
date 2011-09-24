//
//  DataFile.m
//  XNJB
//
//  Created by Richard Low on Sat Aug 21 2004.
//

#import "DataFile.h"
#import "FilesizeFormatter.h"

@implementation DataFile

/* returns a datafile representing the file at path
 */
+ (DataFile *)dataFileFromPath:(NSString *)path
{
	DataFile *dataFile = [[DataFile alloc] init];
	[dataFile setFullPath:path];
	[dataFile setFilename:[path lastPathComponent]];
  [dataFile fileTypeFromExtension];
  
	NSFileManager *fileManager = [[NSFileManager alloc] init];
	NSDictionary *fileAttributes = [fileManager fileAttributesAtPath:path traverseLink:YES];
	if (fileAttributes == nil)
	{
		NSLog(@"Cannot read file %@", path);
		[dataFile release];
    [fileManager release];
		return nil;
	}
	[dataFile setSize:[[fileAttributes objectForKey:NSFileSize] unsignedLongLongValue]];
	//	[dataFile setTimestamp:[fileAttributes objectForKey:NSFileModificationDate]];
  [fileManager release];
	return [dataFile autorelease];
}

// init/dealloc methods

- (void)dealloc
{
	[filename release];
//	[timestamp release];
	[fullPath release];
	[super dealloc];
}

// accessor methods

- (void)setSize:(unsigned long long)newSize
{
	size = newSize;
}

- (unsigned long long)size
{
	return size;
}

- (void)setFilename:(NSString *)newFilename
{
	if (newFilename == nil)
		newFilename = @"";
	// get immutable copy
	newFilename = [NSString stringWithString:newFilename];
	[newFilename retain];
	[filename release];
	filename = newFilename;
}

- (NSString *)filename
{
	return filename;
}

- (NSString *)sizeString
{
	return [FilesizeFormatter filesizeString:size];
}

- (void)setFullPath:(NSString *)newFullPath
{
	if (newFullPath == nil)
		newFullPath = @"";
	// get immutable copy
	newFullPath = [NSString stringWithString:newFullPath];
	[newFullPath retain];
	[fullPath release];
	fullPath = newFullPath;
}

- (NSString *)fullPath
{
	return fullPath;
}

/*- (void)setTimestamp:(NSDate *)newTimestamp
{
	[newTimestamp retain];
	[timestamp release];
	timestamp = newTimestamp;
}

- (void)setTimestampSince1970:(unsigned int)newTimestamp
{
	[self setTimestamp:[NSDate dateWithTimeIntervalSince1970:newTimestamp]];
}

- (NSDate *)timestamp
{
	return timestamp;
}

- (NSString *)timestampString
{
	return [timestamp descriptionWithCalendarFormat:@"%Y-%m-%d %H:%M:%S" timeZone:[NSTimeZone timeZoneForSecondsFromGMT:0] locale:nil];
}*/

/**********************/

- (BOOL)matchesString:(NSString *)search
{
	NSRange result = [filename rangeOfString:search options:NSCaseInsensitiveSearch];
	if (result.location == NSNotFound)
	{
	  return NO;
	}
	return YES;	
}

- (NSString *)description
{
	return filename;
}

/* gets the filetype from the extension of fullPath
 */
- (void)fileTypeFromExtension
{
	[self fileTypeFromExtension:fullPath];
}

@end
