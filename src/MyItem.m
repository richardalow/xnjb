//
//  MyItem.m
//  XNJB
//
//  Created by Richard Low on 07/09/2004.
//

/* This class is used for the 
 * sortable elements of the array.
 * Also declares a method matchesString to see
 * if this item contains a string. This is to be
 * implemented by super classes (e.g. Track or DataFile).
 */

#import "MyItem.h"

@implementation MyItem

- (id)init
{
	if (self = [super init])
	{
		[self setItemID:0];
		[self setFileType:LIBMTP_FILETYPE_UNKNOWN];
	}
	return self;
}

/* To be implemented by super class.
 * Method to search self for NSString search
 */
- (BOOL)matchesString:(NSString *)search
{
	return YES;
}

- (void)setItemID:(unsigned)newItemID
{
	itemID = newItemID;
}

- (unsigned)itemID
{
	return itemID;
}

- (LIBMTP_filetype_t)fileType
{
	return fileType;
}

- (void)setFileType:(LIBMTP_filetype_t)newFileType
{
	fileType = newFileType;
}

- (NSString *)fileTypeString
{
	switch (fileType)
	{
		case LIBMTP_FILETYPE_UNDEF_AUDIO:
		case LIBMTP_FILETYPE_UNKNOWN:
		case LIBMTP_FILETYPE_UNDEF_VIDEO:
			return @"UNKNOWN";
			break;
	  case LIBMTP_FILETYPE_MP3:
			return @"MP3";
			break;
		case LIBMTP_FILETYPE_WMA:
			return @"WMA";
			break;
		case LIBMTP_FILETYPE_WAV:
			return @"WAV";
			break;
		case LIBMTP_FILETYPE_AUDIBLE:
			return @"AA";
			break;
		case LIBMTP_FILETYPE_OGG:
			return @"OGG";
			break;
		case LIBMTP_FILETYPE_MP4:
			return @"MP4";
			break;
		case LIBMTP_FILETYPE_WMV:
			return @"WMV";
			break;
		case LIBMTP_FILETYPE_AVI:
			return @"AVI";
			break;
		case LIBMTP_FILETYPE_MPEG:
			return @"MPEG";
			break;
		case LIBMTP_FILETYPE_ASF:
			return @"ASF";
			break;
		case LIBMTP_FILETYPE_QT:
			return @"QT";
			break;
		case LIBMTP_FILETYPE_JPEG:
			return @"JPEG";
			break;
		case LIBMTP_FILETYPE_JFIF:
			return @"JFIF";
			break;
		case LIBMTP_FILETYPE_TIFF:
			return @"TIFF";
			break;
		case LIBMTP_FILETYPE_BMP:
			return @"BMP";
			break;
		case LIBMTP_FILETYPE_GIF:
			return @"GIF";
			break;
		case LIBMTP_FILETYPE_PICT:
			return @"PICT";
			break;
		case LIBMTP_FILETYPE_PNG:
			return @"PNG";
			break;
		case LIBMTP_FILETYPE_VCALENDAR1:
			return @"VCAL1";
			break;
		case LIBMTP_FILETYPE_VCALENDAR2:
			return @"VCAL2";
			break;
		case LIBMTP_FILETYPE_VCARD2:
			return @"VCARD2";
			break;
		case LIBMTP_FILETYPE_VCARD3:
			return @"VCARD3";
			break;
		case LIBMTP_FILETYPE_WINDOWSIMAGEFORMAT:
			return @"WINDOWSIMAGEFORMAT";
			break;
		case LIBMTP_FILETYPE_WINEXEC:
			return @"EXE";
			break;
		case LIBMTP_FILETYPE_TEXT:
			return @"TXT";
			break;
		case LIBMTP_FILETYPE_HTML:
			return @"HTML";
			break;
		case LIBMTP_FILETYPE_FIRMWARE:
			return @"firmware";
			break;
		case LIBMTP_FILETYPE_AAC:
			return @"AAC";
			break;
		case LIBMTP_FILETYPE_MEDIACARD:
			return @"Mediacard";
			break;
		case LIBMTP_FILETYPE_FLAC:
			return @"FLAC";
			break;
		case LIBMTP_FILETYPE_MP2:
			return @"MP2";
			break;
		case LIBMTP_FILETYPE_M4A:
			return @"M4A";
			break;
		case LIBMTP_FILETYPE_DOC:
			return @"DOC";
			break;
		case LIBMTP_FILETYPE_XML:
			return @"XML";
			break;
		case LIBMTP_FILETYPE_XLS:
			return @"XLS";
			break;
		case LIBMTP_FILETYPE_PPT:
			return @"PPT";
			break;
		case LIBMTP_FILETYPE_MHT:
			return @"MHT";
			break;
		case LIBMTP_FILETYPE_JP2:
			return @"JP2";
			break;
		case LIBMTP_FILETYPE_JPX:
			return @"JPX";
			break;
		case LIBMTP_FILETYPE_FOLDER:
			return @"FOLDER";
			break;
		case LIBMTP_FILETYPE_ALBUM:
			return @"ALBUM";
			break;
		case LIBMTP_FILETYPE_PLAYLIST:
			return @"PLAYLIST";
			break;
	}
	// we can't ever get here...
	NSLog(@"invalid libmtp file type %d", fileType);
	return nil;
}

/* gets the file type from the file extension on fullPath
*/
- (void)fileTypeFromExtension:(NSString *)filename
{
	NSString *extension = [filename pathExtension];
	
	// reverse of fileTypeString
	if ([extension compare:@"MP3" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_MP3];
	else if ([extension compare:@"WMA" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_WMA];
	else if ([extension compare:@"WAV" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_WAV];
	else if ([extension compare:@"AA" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_AUDIBLE];
	else if ([extension compare:@"OGG" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_OGG];
	else if ([extension compare:@"MP4" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_MP4];
	else if ([extension compare:@"WMV" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_WMV];
	else if ([extension compare:@"AVI" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_AVI];
	else if ([extension compare:@"MPG" options:NSCaseInsensitiveSearch] == NSOrderedSame || [extension compare:@"MPEG" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_MPEG];
	else if ([extension compare:@"ASF" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_ASF];
	else if ([extension compare:@"QT" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_QT];
	else if ([extension compare:@"JPG" options:NSCaseInsensitiveSearch] == NSOrderedSame || [extension compare:@"JPEG" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_JPEG];
	else if ([extension compare:@"JFIF" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_JFIF];
	else if ([extension compare:@"TIF" options:NSCaseInsensitiveSearch] == NSOrderedSame || [extension compare:@"TIFF" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_TIFF];
	else if ([extension compare:@"BMP" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_BMP];
	else if ([extension compare:@"GIF" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_GIF];
	else if ([extension compare:@"PICT" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_PICT];
	else if ([extension compare:@"PNG" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_PNG];
	else if ([extension compare:@"EXE" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_WINEXEC];
	else if ([extension compare:@"TXT" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_TEXT];
	else if ([extension compare:@"HTM" options:NSCaseInsensitiveSearch] == NSOrderedSame || [extension compare:@"HTML" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_HTML];
  else if ([extension compare:@"AAC" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_AAC];
  else if ([extension compare:@"FLAC" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_FLAC];
  else if ([extension compare:@"MP2" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_MP2];
  else if ([extension compare:@"M4A" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_MP4]; // At least the Creative Zen needs this
  else if ([extension compare:@"DOC" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_DOC];
  else if ([extension compare:@"XML" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_XML];
  else if ([extension compare:@"XLS" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_XLS];
  else if ([extension compare:@"PPT" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_PPT];
  else if ([extension compare:@"MHT" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_MHT];
  else if ([extension compare:@"JP2" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_JP2];
  else if ([extension compare:@"JPX" options:NSCaseInsensitiveSearch] == NSOrderedSame)
		[self setFileType:LIBMTP_FILETYPE_JPX];
	else 
		[self setFileType:LIBMTP_FILETYPE_UNKNOWN];
}

@end
