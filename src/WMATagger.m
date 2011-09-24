//
//  WMATagger.m
//  XNJB
//
//  Created by Richard Low on 17/09/2004.
//  Copyright 2004 __MyCompanyName__. All rights reserved.
//

#import "WMATagger.h"
#include "wmaread.h"
// for stat
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

@implementation WMATagger

- (id)init
{
	// can't be initialized
	return nil;
}

+ (Track *)readTrack:(NSString *)filename
{
	struct stat sb;
	// check file exists
	if (stat([filename fileSystemRepresentation], &sb) == -1)
		return nil;
	
	// N.B. cannot use NSFileManager as this is not thread safe
		
	Track *track = [[Track alloc] init];
	
	[track setFilesize:sb.st_size];
	[track setFileType:LIBMTP_FILETYPE_WMA];
	[track setFullPath:filename];
	
	metadata_t meta;
	// make sure all pointers are NULL
	memset(&meta, 0, sizeof(metadata_t));
	meta.path = (gchar *)[filename fileSystemRepresentation];
	get_tag_for_wmafile(&meta);
	
	if (meta.title != NULL)
		[track setTitle:[NSString stringWithUTF8String:meta.title]];
	if (meta.artist != NULL)
		[track setArtist:[NSString stringWithUTF8String:meta.artist]];
	if (meta.album != NULL)
		[track setAlbum:[NSString stringWithUTF8String:meta.album]];
	if (meta.genre != NULL)
		[track setGenre:[NSString stringWithUTF8String:meta.genre]];
	[track setTrackNumber:meta.trackno];
	[track setYear:meta.year];
	[track setLength:meta.length];
	return [track autorelease];
}

@end
