//
//  Track.m
//  XNJB
//
//  Created by Richard Low on Sun Jul 18 2004.
//

/* the track object with tag informatioon
 * 
 */
#import "Track.h"
#import "FilesizeFormatter.h"

@implementation Track

// init/dealloc methods

- (id)init
{
	if (self = [super init])
	{
		// set all to sane values
		[self setTitle:@""];
		[self setArtist:@""];
		[self setAlbum:@""];
		[self setGenre:@""];
		//[self setFilename:@""];
		[self setFullPath:@""];
		[self setTrackNumber:0];
		[self setYear:0];
		[self setFilesize:0];
		[self setLength:0];
		[self setFileType:LIBMTP_FILETYPE_UNDEF_AUDIO];
    [self setItcFilePath:@""];
	}
	return self;
}

- (id)initWithTrack:(Track *)track
{
	if (self = [super init])
	{
		[self setValuesToTrack:track];
	}
	return self;
}

- (void)dealloc
{
	[title release];
	[album release];
	[artist release];
	[genre release];
  [filename release];
  [fullPath release];
  [image release];
  [dateAdded release];
  [itcFilePath release];
	[super dealloc];
}

// accessor methods
	
- (NSString *)title
{
	return title;
}

- (void)setTitle:(NSString *)newTitle
{
	if (newTitle == nil)
		newTitle = @"";
	// get immutable copy
	newTitle = [NSString stringWithString:newTitle];
	[newTitle retain];
	[title release];
	title = newTitle;
}

- (NSString *)album
{
	return album;
}

- (void)setAlbum:(NSString *)newAlbum
{
	if (newAlbum == nil)
		newAlbum = @"";
	// get immutable copy
	newAlbum = [NSString stringWithString:newAlbum];
	[newAlbum retain];
	[album release];
	album = newAlbum;
}

- (NSString *)artist
{
	return artist;
}

- (void)setArtist:(NSString *)newArtist
{
	if (newArtist == nil)
		newArtist = @"";
	// get immutable copy
	newArtist = [NSString stringWithString:newArtist];
	[newArtist retain];
	[artist release];
	artist = newArtist;
}

- (NSString *)genre
{
	return genre;
}

- (void)setGenre:(NSString *)newGenre
{
	if (newGenre == nil)
		newGenre = @"";
	// get immutable copy
	newGenre = [NSString stringWithString:newGenre];
	[newGenre retain];
	[genre release];
	genre = newGenre;
}

- (NSString *)filename
{
	return filename;
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

- (NSString *)fullPath
{
	return fullPath;
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

- (unsigned)length
{
	return length;
}

- (NSString *)lengthString
{
	// if length >= 24 hours then we just give the time modulo 24 hours
	NSDate *lengthDate = [NSDate dateWithTimeIntervalSinceReferenceDate:length];
	if (length >= 3600)
	{
		// we have hours
		return [lengthDate descriptionWithCalendarFormat:@"%H:%M:%S" timeZone:[NSTimeZone timeZoneForSecondsFromGMT:0] locale:nil];
	}
	else
		return [lengthDate descriptionWithCalendarFormat:@"%M:%S" timeZone:[NSTimeZone timeZoneForSecondsFromGMT:0] locale:nil];
}

- (void)setLength:(unsigned)newLength
{
	length = newLength;
}

- (unsigned)trackNumber
{
	return trackNumber;
}

- (void)setTrackNumber:(unsigned)newTrackNumber
{
	trackNumber = newTrackNumber;
}

- (NSString *)njbCodec
{
	// is this a sensible default? we should probably set all other than WMA/WAV to MP3
	if (fileType == LIBMTP_FILETYPE_UNDEF_AUDIO || fileType == LIBMTP_FILETYPE_UNKNOWN)
		return @"MP3";
	return [self fileTypeString];
}

- (unsigned long long)filesize
{
	return filesize;
}

- (NSString *)filesizeString
{
	return [FilesizeFormatter filesizeString:filesize];
}

- (void)setYear:(unsigned)newYear
{
	year = newYear;
}

- (unsigned)year
{
	return year;
}

- (void)setFilesize:(unsigned long long)newFilesize
{
	filesize = newFilesize;
}

- (NSString *)itcFilePath
{
	return itcFilePath;
}

- (void)setItcFilePath:(NSString *)newItcFilePath
{
	if (newItcFilePath == nil)
		newItcFilePath = @"";
	// get immutable copy
	newItcFilePath = [NSString stringWithString:newItcFilePath];
	[newItcFilePath retain];
	[itcFilePath release];
	itcFilePath = newItcFilePath;
}

/**********************/

- (void)setValuesToTrack:(Track *)track
{
	[self setTitle:[track title]];
	[self setArtist:[track artist]];
	[self setAlbum:[track album]];
	[self setGenre:[track genre]];
	[self setFilename:[track filename]];
	[self setFullPath:[track fullPath]];
	[self setTrackNumber:[track trackNumber]];
	[self setYear:[track year]];
	[self setFilesize:[track filesize]];
	[self setLength:[track length]];
	[self setFileType:[track fileType]];
	[self setItemID:[track itemID]];
  [self setImage:[track image]];
  [self setItcFilePath:[track itcFilePath]];
}

- (NSString *)description
{
	return [NSString stringWithFormat:@"Title: %@, Artist: %@, Album: %@, Genre: %@, Length: %@, Track No: %d, File Type: %@, Filesize: %@, Track ID: %d, Year: %d, Filename: %@, Fullpath: %@",
		title, artist, album, genre, [self lengthString], trackNumber,
		[self fileTypeString], [self filesizeString], itemID, year, filename, fullPath];
}

/* searches title, album, artist and genre for the string
 * search. Case insensitive
 */
- (BOOL)matchesString:(NSString *)search
{
	NSRange result = [title rangeOfString:search options:NSCaseInsensitiveSearch];
	if (result.location == NSNotFound)
	{
		result = [album rangeOfString:search options:NSCaseInsensitiveSearch];
		if (result.location == NSNotFound)
		{
			result = [artist rangeOfString:search options:NSCaseInsensitiveSearch];
			if (result.location == NSNotFound)
			{
				result = [genre rangeOfString:search options:NSCaseInsensitiveSearch];
				if (result.location == NSNotFound)
					return NO;
			}
		}
	}
	return YES;	
}

- (NSComparisonResult)compareByLength:(Track *)other
{
	return [self compareUnsignedInts:length withOther:[other length]];
}

/* this compares the two unsigned ints mine and theirs
 */
- (NSComparisonResult)compareUnsignedInts:(unsigned)mine withOther:(unsigned)theirs
{
	if (mine == theirs)
			return NSOrderedSame;
	else if (mine < theirs)
		return NSOrderedAscending;
	else
		return NSOrderedDescending;
}

/* gets the filetype from the extension of fullPath
 */
- (void)fileTypeFromExtension
{
	[self fileTypeFromExtension:fullPath];
}

- (void)setImage:(NSBitmapImageRep *)newImage
{
  [newImage retain];
  [image release];
  image = newImage;
}

- (NSBitmapImageRep *)image;
{
  // A lazy evaluation if we have an itc file
  // we don't save it since it uses a lot of memory
  if (image != nil)
    return image;
    
  if ([itcFilePath isEqualToString:@""])
    return nil;
    
  NSData *itcData = [[NSFileManager defaultManager] contentsAtPath:itcFilePath];
  if (itcData == nil)
  {
    NSLog(@"cannot read itc file %@", itcFilePath);
    return nil;
  }
  NSRange range;
  range.location = 0x1ec;
  range.length = [itcData length] - range.location;
  NSData *imageData = [itcData subdataWithRange:range];
  
  return [NSBitmapImageRep imageRepWithData:imageData];
}

- (void)setDateAdded:(NSDate *)newDateAdded
{
  [newDateAdded retain];
  [dateAdded release];
  dateAdded = newDateAdded;
}

- (NSDate *)dateAdded
{
  return dateAdded;
}

- (void)setRating:(unsigned int)newRating
{
  rating = newRating;
}

- (unsigned int)rating
{
  return rating;
}


@end
