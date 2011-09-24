//
//  Track.h
//  XNJB
//
//  Created by Richard Low on Sun Jul 18 2004.
//

#import <Foundation/Foundation.h>
#import "MyItem.h"

@interface Track : MyItem {
@private
  NSString *title;
	NSString *album;
	NSString *artist;
	NSString *genre;
	NSString *filename;
	NSString *fullPath;
	unsigned length;
	unsigned trackNumber;
	unsigned year;
	unsigned long long filesize;
  NSBitmapImageRep *image;
  NSString *itcFilePath;
  NSDate *dateAdded;
  int rating;
}
- (id)initWithTrack:(Track *)track;
- (NSString *)title;
- (void)setTitle:(NSString *)newTitle;
- (NSString *)album;
- (void)setAlbum:(NSString *)newAlbum;
- (NSString *)artist;
- (void)setArtist:(NSString *)newArtist;
- (NSString *)genre;
- (void)setGenre:(NSString *)newGenre;
- (NSString *)filename;
- (void)setFilename:(NSString *)newFilename;
- (NSString *)fullPath;
- (void)setFullPath:(NSString *)newFullPath;
- (unsigned)length;
- (NSString *)lengthString;
- (void)setLength:(unsigned)newLength;
- (unsigned)trackNumber;
- (void)setYear:(unsigned)newYear;
- (unsigned)year;
- (void)setTrackNumber:(unsigned)newTrackNumber;
- (NSString *)njbCodec;
- (NSString *)itcFilePath;
- (void)setItcFilePath:(NSString *)newItcFilePath;
- (unsigned long long)filesize;
- (NSString *)filesizeString;
- (void)setFilesize:(unsigned long long)newFilesize;
- (void)setValuesToTrack:(Track *)track;
- (void)fileTypeFromExtension;
- (void)setImage:(NSBitmapImageRep *)newImage;
- (NSBitmapImageRep *)image;
- (void)setDateAdded:(NSDate *)newDateAdded;
- (NSDate *)dateAdded;
- (void)setRating:(unsigned int)newRating;
- (unsigned int)rating;

- (NSComparisonResult)compareByLength:(Track *)other;
- (NSComparisonResult)compareUnsignedInts:(unsigned)mine withOther:(unsigned)theirs;

@end
