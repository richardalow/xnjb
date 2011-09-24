//
//  Album.h
//  XNJB
//
//  Created by Richard Low on 03/03/2007.
//

#import <Playlist.h>
#include <libmtp.h>

@interface Album : Playlist {

}
- (void)setAlbumID:(unsigned)newAlbumID;
- (unsigned)albumID;
- (LIBMTP_album_t *)toLIBMTPAlbum:(Track *)track;

@end
