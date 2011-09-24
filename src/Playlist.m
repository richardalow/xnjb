//
//  Playlist.m
//  XNJB
//
//  Created by Richard Low on 06/09/2004.
//

/* This class represents a playlist with a name,
 * tracks and state.
 * Because this object is used in both threads
 * it needs to be locked before sending any 
 * messages by sending the lock message. When
 * finished, send the unlock message.
 * lock will block until the lock can be obtained.
 */

#import "Playlist.h"
// for the playlist state defs
#import "libnjb.h"

// the default name for a new playlist
#define DEFAULT_PLAYLIST_NAME @"New Playlist"

@implementation Playlist

// init/dealloc methods

- (id)init
{
	if (self = [super init])
	{
		tracks = [[NSMutableArray alloc] init];
		lock = [[NSLock alloc] init];
		[self setName:DEFAULT_PLAYLIST_NAME];
		state = NJB_PL_NEW;
	}
	return self;
}

- (void)dealloc
{
	[tracks release];
	[name release];
	[lock release];
	[super dealloc];
}

// accessor methods

/* state is used to know what has been changed 
 * so we know what to upload to the Jukebox.
 * See playlist.c in libnjb
 * setState should not normally be called as the
 * state is automatically updated. 
 */
- (void)setState:(unsigned)newState
{
	state = newState;
}

- (unsigned)state
{
	return state;
}

- (void)setPlaylistID:(unsigned)newPlaylistID
{
	playlistID = newPlaylistID;
}

- (unsigned)playlistID
{
	return playlistID;
}

/* Returns the array of tracks.
 * a bit nasty as this can be freed and be modified through here without updating our status...
 */
- (NSMutableArray *)tracks
{
	return tracks;
}

- (NSString *)name
{
	return name;
}

- (void)setName:(NSString *)newName
{
	if (newName == nil)
		newName = @"";
	// get immutable copy
	newName = [NSString stringWithString:newName];
	[newName retain];
	[name release];
	name = newName;
	if (state == NJB_PL_UNCHANGED)
		state = NJB_PL_CHNAME;
}

- (unsigned)trackCount
{
	return [tracks count];
}

// Track functions

/* adds a track to the array
 * and updates status
 */
- (void)addTrack:(Track *)track
{
	[tracks addObject:track];
	if (state != NJB_PL_NEW)
		state = NJB_PL_CHTRACKS;
}

/* deletes the tracks in the array tracksToDelete
 * Use this when you want to delete >1 tracks as
 * indexes change when tracks are deleted
 * updates the status
 */
- (void)deleteTracks:(NSArray *)tracksToDelete
{
	NSEnumerator *trackEnumerator = [tracksToDelete objectEnumerator];
	Track *currentTrack;
	while (currentTrack = [trackEnumerator nextObject])
	{
		unsigned index = [tracks indexOfObject:currentTrack];
		
		if (index == NSNotFound)
			NSLog(@"track not found in deleteTracks:!");
		else
			[tracks removeObjectAtIndex:index];
	}
	if (state != NJB_PL_NEW)
		state = NJB_PL_CHTRACKS;
}

- (void)deleteTrack:(Track *)track
{
	[tracks removeObject:track];
	if (state != NJB_PL_NEW)
		state = NJB_PL_CHTRACKS;
}

- (void)deleteTrackWithID:(unsigned)trackID
{
  MyItem *item = [tracks itemWithID:trackID];
  if (item != nil)
    [tracks removeObject:item];

  if (state != NJB_PL_NEW)
  state = NJB_PL_CHTRACKS;
}

/* replaces the Tracks containing just IDs into full Track objects
 * from allTracks
 */
- (void)fillTrackInfoFromList:(NSMutableArray *)allTracks
{
	Track *currentTrack;
	int index = 0;
  for (index = 0; index < [tracks count]; index++)
  {
    currentTrack = [tracks objectAtIndex:index];
		Track *fullTrack = (Track *)[allTracks itemWithID:[currentTrack itemID]];
		if (fullTrack == nil)
		{
			// if not found assume does not exist anymore so delete
			NSLog(@"itemID not recognised: %d", [currentTrack itemID]);
			[tracks removeObjectAtIndex:index];
      index--;
		}
		else
		{
			[tracks replaceObjectAtIndex:index withObject:fullTrack];
		}
	}
}

- (NSString *)description
{
	return [NSString stringWithFormat:@"Name: %@, ID: %u, Tracks: %u, State: %d, track info: \n%@", name, playlistID, [tracks count], state, tracks];
}

/* This is called when reordering playlists
 * swaps the track at index with the track
 * at index-1
 */
- (void)swapTrackWithPreviousAtIndex:(unsigned)index
{
	if (index == 0)
	{
		NSLog(@"index == 0 in swapTrackWithPreviousAtIndex:!");
		return;
	}
	[self swapTrackWithNextAtIndex:(index - 1)];
}

/* This is called when reordering playlists
 * swaps the track at index with the track
 * at index+1
 */
- (void)swapTrackWithNextAtIndex:(unsigned)index
{
	if (index == [self trackCount]-1)
	{
		NSLog(@"index == [self trackCount]-1 in swapTrackWithNextAtIndex:!");
		return;
	}
	[tracks exchangeObjectAtIndex:index withObjectAtIndex:(index + 1)];
	if (state != NJB_PL_NEW)
		state = NJB_PL_CHTRACKS;
}

/* the lock functions that must be called
 * before reading/modifying this object
 * if it could be accesses from both threads
 */
- (void)lock
{
	[lock lock];
}

- (void)unlock
{
	[lock unlock];
}

@end
