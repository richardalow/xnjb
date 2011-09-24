//
//  Playlist.h
//  XNJB
//
//  Created by Richard Low on 06/09/2004.
//

#import <Cocoa/Cocoa.h>
#import "Track.h"
#import "MyItem.h"

@interface Playlist : MyItem {
	unsigned playlistID;
	NSMutableArray *tracks;
	NSString *name;
	unsigned state;
	NSLock *lock;
}
- (void)setState:(unsigned)newState;
- (unsigned)state;
- (void)setPlaylistID:(unsigned)newPlaylistID;
- (unsigned)playlistID;
- (void)addTrack:(Track *)track;
- (void)deleteTrack:(Track *)track;
- (void)deleteTracks:(NSArray *)tracksToDelete;
- (NSMutableArray *)tracks;
- (NSString *)name;
- (void)setName:(NSString *)newName;
- (void)fillTrackInfoFromList:(NSMutableArray *)allTracks;
- (unsigned)trackCount;
- (void)swapTrackWithPreviousAtIndex:(unsigned)index;
- (void)swapTrackWithNextAtIndex:(unsigned)index;
- (void)lock;
- (void)unlock;
- (void)deleteTrackWithID:(unsigned)trackID;
@end
