//
//  Album.m
//  XNJB
//
//  Created by Richard Low on 03/03/2007.
//

#import "Album.h"


@implementation Album

- (void)setAlbumID:(unsigned)newAlbumID
{
  [super setPlaylistID:newAlbumID]; 
}

- (unsigned)albumID
{
  return [super playlistID];
}

// returned object must be freed by caller
// needs a track object to get metadata
- (LIBMTP_album_t *)toLIBMTPAlbum:(Track *)track
{
  uint32_t size = [tracks count];
  uint32_t *trackArray = (uint32_t*)malloc(sizeof(uint32_t)*size);
  uint32_t i = 0;
  for (i = 0; i < size; i++)
    trackArray[i] = [[tracks objectAtIndex:i] itemID];
		
  LIBMTP_album_t *libmtpAlbum = LIBMTP_new_album_t();
  libmtpAlbum->name = strdup([name UTF8String]);
  libmtpAlbum->artist = strdup([[track artist] UTF8String]);
  libmtpAlbum->genre = strdup([[track genre] UTF8String]);
  libmtpAlbum->album_id = [self albumID];
  libmtpAlbum->no_tracks = size;
  libmtpAlbum->tracks = trackArray;
  
  return libmtpAlbum;
}


@end
