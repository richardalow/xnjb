//
//  ID3Tagger.m
//  id3libtest
//
//  Created by Richard Low on Mon Jul 26 2004.
//

/* this class provides a wrapper for libid3tag
 * to read/write id3 tags
 */

#include "wmaread.h"
#import "ID3Tagger.h"
#import "WMATagger.h"
#import "UnicodeWrapper.h"
#import "MP3Length.h"
// for stat
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <taglib.h>
#include <audioproperties.h>
#include <tstring.h>
#include <fileref.h>
#include <tag.h>
#include <mpegfile.h>
#include <id3v2tag.h>
//#include <id3v2frame.h>
//#include <id3v2header.h>
#include <attachedpictureframe.h>

using namespace std;
using namespace TagLib;

//#import "TagAPI.h"

@interface ID3Tagger (PrivateAPI)
- (NSString *)trimTagString:(NSString *)string;
- (unsigned)wavFileLength:(NSString *)path;
@end

@implementation ID3Tagger

// init/dealloc methods

- (id)init
{
	return [self initWithFilename:nil];
}

- (id)initWithFilename:(NSString *)newFilename
{
	if (self = [super init])
	{
		[self setFilename:newFilename];
	}
	return self;
}

- (void)dealloc
{
	[filename release];
	[super dealloc];
}

// accessor methods

- (void)setFilename:(NSString *)newFilename
{
	[newFilename retain];
	[filename release];
	filename = newFilename;
}

/**********************/


/* read the id3 tag and return it as
 * a track object
 * will pass on to WMATagger if a wma file
 */
- (Track *)readTrack
{	
	Track *track = [[Track alloc] init];
	
	[track setFullPath:filename];
	[track fileTypeFromExtension];
	
	// if is WMA file, we can't do anything here, pass on to WMATagger
	if ([track fileType] == LIBMTP_FILETYPE_WMA)
	{
		[track release];
		return [WMATagger readTrack:filename];
	}
	
	struct stat sb;
	// check file exists
	if (stat([filename fileSystemRepresentation], &sb) == -1)
		return nil;
	
	// N.B. cannot use NSFileManager as this is not thread safe
	
	[track setFilesize:sb.st_size];
	
  if ([track fileType] == LIBMTP_FILETYPE_MP3)
	{
    TagLib::FileRef file([filename UTF8String]);
    if (!file.isNull() && file.tag())
    {
      TagLib::Tag *tag = file.tag();
      
      // todo: check unicode formatting
      [track setTitle:[self trimTagString:[NSString stringWithUTF8String:tag->title().toCString(true)]]];
      [track setArtist:[self trimTagString:[NSString stringWithUTF8String:tag->artist().toCString(true)]]];
      [track setAlbum:[self trimTagString:[NSString stringWithUTF8String:tag->album().toCString(true)]]];
      [track setGenre:[self trimTagString:[NSString stringWithUTF8String:tag->genre().toCString(true)]]];
      [track setTrackNumber:tag->track()];
      [track setYear:tag->year()];
      
      TagLib::AudioProperties *properties = file.audioProperties();
      
      [track setLength:properties->length()];
      
      // now do image - have to assume is MPEG here
      MPEG::File mpegFile([filename UTF8String]);
      
      ID3v2::Tag *id3v2Tag = mpegFile.ID3v2Tag(false);
      
      if (id3v2Tag)
      {
        TagLib::ID3v2::FrameList frameList = id3v2Tag->frameListMap()["APIC"];
        if (!frameList.isEmpty())
        {
          TagLib::ID3v2::AttachedPictureFrame *p = static_cast<TagLib::ID3v2::AttachedPictureFrame *>(frameList.front());
          
          NSData *data = [NSData dataWithBytes:p->picture().data() length:p->picture().size()];
          NSBitmapImageRep *image = [[NSBitmapImageRep alloc] initWithData:data];
          [track setImage:image];
          [image release];
        }
      }
    }
	}
	else if ([track fileType] == LIBMTP_FILETYPE_WAV)
	{
		// use filename as title, but strip extension
		NSString *title = [[filename lastPathComponent] stringByDeletingPathExtension];
		[track setTitle:title];
		[track setLength:[self wavFileLength:filename]];
	}
	
	return [track autorelease];
}

/* write the tag in track to the file
 * in [track fullPath]
 */
- (void)writeTrack:(Track *)track
{
/*	NSMutableDictionary * GenreDictionary = [[NSMutableDictionary alloc] initWithContentsOfFile: @"~/GenreDictionary.plist"];
	TagAPI *tag = [[TagAPI alloc] initWithGenreList: GenreDictionary];
	[tag examineFile:[track fullPath]];

	[tag setTitle:[track title]];
	[tag setArtist:[track artist]];
  [tag setAlbum:[track album]];
	[tag setGenreName:[NSArray arrayWithObject:[track genre]]];
	int totalTracks = [tag getTotalNumberTracks];
	if (totalTracks != -1)
		[tag setTrack:[track trackNumber] totalTracks:[tag getTotalNumberTracks]];
	else
		[tag setTrack:[track trackNumber] totalTracks:0];
	
	[tag setYear:[track year]];
	
	[tag updateFile];
	
	[tag release];	*/
  
  TagLib::FileRef file([[track fullPath] UTF8String]);
  if (!file.isNull() && file.tag())
  {
    TagLib::Tag *tag = file.tag();
   
    String strTitle = String([[track title] UTF8String], String::UTF8);
    tag->setTitle(strTitle);
    String strArtist = String([[track artist] UTF8String], String::UTF8);
    tag->setArtist(strArtist);
    String strAlbum = String([[track album] UTF8String], String::UTF8);
    tag->setAlbum(strAlbum);
    String strGenre = String([[track genre] UTF8String], String::UTF8);
    tag->setGenre(strGenre);

    tag->setYear([track year]);
    
    file.save();
    
    // now for track - need to preserve total number of tracks
    MPEG::File mpegFile([[track fullPath] UTF8String]);
      
    ID3v2::Tag *id3v2Tag = mpegFile.ID3v2Tag(false);
      
    if (id3v2Tag)
    {
      TagLib::ID3v2::FrameList frameList = id3v2Tag->frameListMap()["TRCK"];
      if (!frameList.isEmpty())
      {
        TagLib::ID3v2::Frame *frame = static_cast<TagLib::ID3v2::Frame *>(frameList.front());
        
        NSString *trackString = [NSString stringWithUTF8String:frame->toString().toCString()];
        NSArray *components = [trackString componentsSeparatedByString:@"/"];
        if ([components count] == 2)
        {
          NSString *total = [components objectAtIndex:1];
          trackString = [NSString stringWithFormat:@"%u/%@", [track trackNumber], total];
          
          frame->setText([trackString UTF8String]);
        }
        else
        {
          id3v2Tag->setTrack([track trackNumber]);
        }
      }
      else
      {
        id3v2Tag->setTrack([track trackNumber]);
      }
      mpegFile.save();
    }
    else
    {
      tag->setTrack([track trackNumber]);
      file.save();
    }
  }
}

/* get the length of the wav file at path
 * see http://ccrma.stanford.edu/courses/422/projects/WaveFormat/ for wave format spec
 */
- (unsigned)wavFileLength:(NSString *)path
{
	// open the file handle for the specified path
  NSFileHandle *file = [NSFileHandle fileHandleForReadingAtPath:path];
  if (file == nil)
  {
		NSLog(@"Cannot open file :%@", path);
    return 0;
	}
	
	[file seekToFileOffset:28];
	
	NSData *buffer = [file readDataOfLength:4];
	const unsigned char *bytesPerSecondChar = (const unsigned char*)[buffer bytes];
	unsigned int bytesPerSecond = bytesPerSecondChar[0] + (bytesPerSecondChar[1] << 8) + (bytesPerSecondChar[2] << 16) + (bytesPerSecondChar[3] << 24);
	
	[file seekToFileOffset:40];
	
	buffer = [file readDataOfLength:4];
	const unsigned char *bytesLongChar = (const unsigned char*)[buffer bytes];
	unsigned int bytesLong = bytesLongChar[0] + (bytesLongChar[1] << 8) + (bytesLongChar[2] << 16) + (bytesLongChar[3] << 24);
	
	[file closeFile];
	
	if (bytesPerSecond == 0)
	{
		NSLog(@"bytesPerSecond == 0 in file %@", path);
		return 0;
	}
	// round up to be on the safe side
	if (bytesLong % bytesPerSecond == 0)
		return (bytesLong / bytesPerSecond);
	else
		return (bytesLong / bytesPerSecond) + 1;
}

- (NSString *)trimTagString:(NSString *)string
{
	if (string == nil || [string length] < 1)
		return nil;
	
	while ([string length] > 0 && [string characterAtIndex:([string length] - 1)] == 0)
	{
		string = [string substringToIndex:([string length] - 1)];
	}
	
	return string;
}

@end
