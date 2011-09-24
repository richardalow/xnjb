//
//  PlaylistsTab.m
//  XNJB
//

/* the playlists tab. Supports creating, removing
 * and modifying playlists.
 */

#import "PlaylistsTab.h"
#import "Playlist.h"
#import "defs.h"

// these correspond to the identifiers in the NIB file
#define COLUMN_TITLE @"Title"
#define COLUMN_ARTIST @"Artist"
#define COLUMN_ALBUM @"Album"
#define COLUMN_GENRE @"Genre"
#define COLUMN_LENGTH @"Length"
#define COLUMN_TRACK_NO @"Track No"
#define COLUMN_CODEC @"Codec"
#define COLUMN_FILESIZE @"Filesize"
#define COLUMN_YEAR @"Year"

// declare the private methods
@interface PlaylistsTab (PrivateAPI)
- (void)fillPlaylistTrackInfo;
- (void)allowNewPlaylist;
- (void)swapPlaylistTrackWithPreviousAtIndex:(unsigned)index forPlaylist:(Playlist *)playlist;
- (void)swapPlaylistTrackWithNextAtIndex:(unsigned)index forPlaylist:(Playlist *)playlist;
@end

@implementation PlaylistsTab

// init/dealloc methods

- (void)dealloc
{
	[playlists release];
	[super dealloc];
}

/**********************/

- (void)onConnectAndActive
{
	[super onConnectAndActive];
	if (!trackListUpToDate)
		[self loadTracks];
	[self loadPlaylists];
}

- (void)activate
{
	// this must be called first to run onConnectAndActive so we can download the tracks there
	[super activate];
	if (!trackListUpToDate)
	{
		[self loadTracks];
	}
}

- (id)tableView:(NSTableView *)aTableView
   objectValueForTableColumn:(NSTableColumn *)aTableColumn
						row:(int)rowIndex
{	
	if (aTableView == playlistsTable)
	{
		Playlist *playlist = (Playlist *)[playlists objectAtIndex:rowIndex];
	  return [playlist name];
	}
	else if (aTableView == playlistsTrackTable)
	{
		if (tracksInCurrentPlaylist != nil)
		{
			NSString *ident = [aTableColumn identifier];
			// this may happen if the IDs we got from the playlist were invalid
			if ([tracksInCurrentPlaylist count] <= rowIndex)
				return nil;
			Track *track = (Track *)[tracksInCurrentPlaylist objectAtIndex:rowIndex];
			if ([ident isEqual:COLUMN_TITLE])
				return [track title];
			else if ([ident isEqual:COLUMN_ARTIST])
				return [track artist];
			else
			{
				NSLog(@"Invalid column tag in PlaylistsTab for playlistsTrackTable");
				return nil;
			}
		}
		else
			return nil;
	}
	//NSLog(@"Unknown table %@ in PlaylistsTab tableView:objectValueForTableColumn:row:!", aTableView);
	return nil;
}

- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
	if (aTableView == playlistsTable)
		return [playlists count];
	else if (aTableView == playlistsTrackTable)
	{
		if (tracksInCurrentPlaylist)
			return [tracksInCurrentPlaylist count];
		else
			return 0;
	}
	// may get here if any of the table pointers above haven't been set yet
	//NSLog(@"Unknown table %@ in PlaylistsTab numberOfRowsInTableView:", aTableView);
	return 0;
}

/* add downloadTrackList to the njb queue
 */
- (void)loadTracks
{
	if (trackListUpToDate)
	{
		NSLog(@"loadTracks called in Playlists tab when track list up to date");
		return;
	}
	NSMutableArray *cachedTrackList = [theNJB cachedTrackList];
	if (cachedTrackList != nil)
	{
		[tempArray release];
		tempArray = cachedTrackList;
		[tempArray retain];
		[self showNewArray];
		
		trackListUpToDate = YES;
		return;
	}
	
	NJBQueueItem *getTracks = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(downloadTrackList)];
	[getTracks setStatus:STATUS_DOWNLOADING_TRACK_LIST];
	
	NJBQueueItem *updateTable = [[NJBQueueItem alloc] initWithTarget:self
																											withSelector:@selector(showNewArray)
																												withObject:nil
																							 withRunInMainThread:YES];
	[updateTable setDisplayStatus:NO];
	
	NSString *description = NSLocalizedString(@"Downloading Track List", nil);
	[self addToQueue:getTracks withSubItems:[NSArray arrayWithObjects:updateTable, nil] withDescription:description];
	[getTracks release];
	[updateTable release];
	// it's not actually yet, but will be in a while, don't want to go copying anything
	trackListUpToDate = YES;
}

/* this is called from the queue consumer 
 * to get the tracks off the NJB and put them in fullItemArray
 * will run in worker thread
 */
- (NJBTransactionResult *)downloadTrackList
{
	[tempArray release];
	tempArray = [theNJB tracks];
	[tempArray retain];
	
	NJBTransactionResult *result = [[NJBTransactionResult alloc] initWithSuccess:(tempArray != nil)];
	
	trackListUpToDate = YES;
	
	return [result autorelease];
}

/* add downloadPlaylists to the njb queue
 */
- (void)loadPlaylists
{
	NJBQueueItem *getPlaylists = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(downloadPlaylists)];
	[getPlaylists setStatus:STATUS_DOWNLOADING_PLAYLISTS];
	
	NJBQueueItem *fillTrackInfo = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(fillPlaylistTrackInfo)
																													withObject:nil withRunInMainThread:YES];
	[fillTrackInfo setDisplayStatus:NO];
	
	NJBQueueItem *updateTable = [[NJBQueueItem alloc] initWithTarget:playlistsTable
																											withSelector:@selector(reloadData)
																												withObject:nil
																							 withRunInMainThread:YES];
	[updateTable setDisplayStatus:NO];
	
	NJBQueueItem *allowNewPlaylist = [[NJBQueueItem alloc] initWithTarget:self
																											withSelector:@selector(allowNewPlaylist)
																												withObject:nil
																							 withRunInMainThread:YES];
	[allowNewPlaylist setDisplayStatus:NO];
	
	NSString *description = NSLocalizedString(@"Getting playlists", nil);
	[self addToQueue:getPlaylists withSubItems:[NSArray arrayWithObjects:fillTrackInfo, updateTable, allowNewPlaylist, nil] withDescription:description];
	[getPlaylists release];
	[fillTrackInfo release];
	[updateTable release];
	[allowNewPlaylist release];
}

/* this is called from the queue consumer 
 * to get the playlists off the NJB and put them in playlists
 * will run in worker thread
 */
- (NJBTransactionResult *)downloadPlaylists
{
	[playlists release];
	
	playlists = [theNJB playlists];
		
	NJBTransactionResult *result = [[NJBTransactionResult alloc] initWithSuccess:(playlists != nil)];
		
	[playlists retain];
  
  NSLog(@"leaving downloadPlaylists");
		
	return [result autorelease];
}

/* maps track ids to full tag information from
 * fullItemArray
 */
- (void)fillPlaylistTrackInfo
{
  NSLog(@"enter fillPlaylistTrackInfo");
	NSEnumerator *enumerator = [playlists objectEnumerator];
	Playlist *currentPlaylist;
	while (currentPlaylist = [enumerator nextObject])
	{
		[currentPlaylist fillTrackInfoFromList:[arrayController content]];
	}
}

/* enable the new playlist button
 */
- (void)allowNewPlaylist
{
	[newPlaylistButton setEnabled:YES];
}

/* update the playlist on the jukebox
 */
- (void)updateJukeboxPlaylist:(Playlist *)playlist
{
	NJBQueueItem *updatePlaylist = [[NJBQueueItem alloc] initWithTarget:theNJB withSelector:@selector(updatePlaylist:) withObject:playlist];
	[updatePlaylist setStatus:STATUS_UPDATING_PLAYLIST];
	
	NSString *description = [NSString stringWithFormat:NSLocalizedString(@"Updating playlist %@", nil), [playlist name]];
	[self addToQueue:updatePlaylist withSubItems:nil withDescription:description];
	[updatePlaylist release];
}

/* delete playlist playlist from the NJB
 */
- (void)deleteJukeboxPlaylist:(Playlist *)playlist
{	
	NJBQueueItem *deletePlaylist = [[NJBQueueItem alloc] initWithTarget:theNJB withSelector:@selector(deletePlaylist:) withObject:playlist];
	[deletePlaylist setStatus:STATUS_DELETING_PLAYLIST];
	
	NSString *description = [NSString stringWithFormat:NSLocalizedString(@"Deleting playlist %@", nil), [playlist name]];
	[self addToQueue:deletePlaylist withSubItems:nil withDescription:description];
	[deletePlaylist release];
}

/* add the selected tracks in njbTable to the selected
 * playlist
 */
- (IBAction)addToPlaylist:(id)sender
{
	NSArray *selectedObjects = [arrayController selectedObjects];
	NSEnumerator *trackEnumerator = [selectedObjects objectEnumerator];
	Track *track;
	Playlist *playlist = (Playlist *)[playlists objectAtIndex:[playlistsTable selectedRow]];
	[playlist lock];
	while (track = (Track *)[trackEnumerator nextObject])
	{
		// this is added at the end so don't worry about indexes changing
		[playlist addTrack:track];
	}
	[playlist unlock];
	// update the jukebox
	[self updateJukeboxPlaylist:playlist];
	[playlistsTrackTable reloadData];
}

/* delete the selected tracks in playlistsTrackTable from the selected
 * playlist
 */
- (IBAction)deleteFromPlaylist:(id)sender
{
	NSEnumerator *trackEnumerator = [playlistsTrackTable selectedRowEnumerator];
	NSNumber *index;
	Playlist *playlist = (Playlist *)[playlists objectAtIndex:[playlistsTable selectedRow]];
	NSMutableArray *tracksToDelete = [[NSMutableArray alloc] initWithCapacity:[playlistsTrackTable numberOfSelectedRows]];
	NSArray *tracks = [playlist tracks];
	while (index = [trackEnumerator nextObject])
	{
		[tracksToDelete addObject:[tracks objectAtIndex:[index unsignedIntValue]]];
	}
	[playlist lock];
	[playlist deleteTracks:tracksToDelete];
	[playlist unlock];
	// update the jukebox
	[self updateJukeboxPlaylist:playlist];
	[tracksToDelete release];
	[playlistsTrackTable reloadData];
}

/* create a new playlist
 */
- (IBAction)newPlaylist:(id)sender
{
	Playlist *playlist = [[Playlist alloc] init];
	// this adds an object to the end
	[playlists addObject:playlist];
	[playlist release];
	[playlistsTable reloadData];
	// so select the last row for editing
	int row = ([playlists count] - 1);
	[playlistsTable selectRow:row byExtendingSelection:NO];
	[playlistsTable editColumn:0 row:row withEvent:nil select:YES];
}

/* delete the selected playlist
 */
- (IBAction)deletePlaylist:(id)sender
{
	int result = NSRunAlertPanel(NSLocalizedString(@"Playlist Deletion", nil),
															 NSLocalizedString(@"Are you sure you want to delete the selected playlist off the NJB?", nil),
															 NSLocalizedString(@"No", nil),
															 NSLocalizedString(@"Yes", nil), nil);
	if (result == NSAlertDefaultReturn)
		return;
	
	int row = [playlistsTable selectedRow];
	// update the jukebox first so not released
	[self deleteJukeboxPlaylist:(Playlist *)[playlists objectAtIndex:row]];
	[playlists removeObjectAtIndex:row];
	tracksInCurrentPlaylist = nil;
	
	[playlistsTable reloadData];
}

- (void)NJBConnected:(NSNotification *)note
{
	trackListUpToDate = NO;
	
	tracksInCurrentPlaylist = nil;
	
	[playlists release];
	playlists = nil;
	[playlistsTable reloadData];
	[playlistsTrackTable reloadData];
	if ([playlistsTable selectedRow] != -1)
		[deletePlaylistButton setEnabled:YES];
	if ([playlistsTrackTable selectedRow] != -1)
		[deleteFromPlaylistButton setEnabled:YES];
	if ([njbTable selectedRow] != -1 && [playlistsTable selectedRow] != -1)
		[addToPlaylistButton setEnabled:YES];
	[super NJBConnected:note];
}

- (void)NJBDisconnected:(NSNotification *)note
{
	trackListUpToDate = NO;
	
	[newPlaylistButton setEnabled:NO];
	[deletePlaylistButton setEnabled:NO];
	[addToPlaylistButton setEnabled:NO];
	[deleteFromPlaylistButton setEnabled:NO];
	[moveTracksUpButton setEnabled:NO];
	[moveTracksDownButton setEnabled:NO];
	[super NJBDisconnected:note];
}

- (void)tableView:(NSTableView *)aTableView setObjectValue:(id)anObject forTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex
{
	if (aTableView != playlistsTable)
		return;
	Playlist *playlist = (Playlist *)[playlists objectAtIndex:rowIndex];
	[playlist lock];
	[playlist setName:anObject];
	[playlist unlock];
	// update the jukebox
	[self updateJukeboxPlaylist:playlist];
}

- (void)tableViewSelectionDidChange:(NSNotification *)aNotification
{
	[super tableViewSelectionDidChange:aNotification];
	// don't necessarily have to disable all buttons if not connected
	// as will have been disabled in NJBDisconnected:
	if ([aNotification object] == njbTable)
	{
		if ([njbTable selectedRow] == -1 || [playlistsTable selectedRow] == -1)
			[addToPlaylistButton setEnabled:NO];
		else if ([theNJB isConnected])
		{
			[addToPlaylistButton setEnabled:YES];
		}
	}
	else if ([aNotification object] == playlistsTable)
	{
		int row = [playlistsTable selectedRow];
		if (row == -1)
		{
			[addToPlaylistButton setEnabled:NO];
			[deletePlaylistButton setEnabled:NO];
			
			tracksInCurrentPlaylist = nil;
			
			[playlistsTrackTable reloadData];
		}
		else
		{
		  // update playlistsTrackTable
			// no need to retain this as is not released until playlists is dealloced
			Playlist *playlist = (Playlist *)[playlists objectAtIndex:[playlistsTable selectedRow]];
			tracksInCurrentPlaylist = [playlist tracks];
			[playlistsTrackTable reloadData];
			
			if ([theNJB isConnected])
			{
				[deletePlaylistButton setEnabled:YES];
				if ([njbTable selectedRow] != -1)
					[addToPlaylistButton setEnabled:YES];
			}
		}
	}
	else if ([aNotification object] == playlistsTrackTable)
	{
		if ([playlistsTrackTable selectedRow] == -1)
		{
			[deleteFromPlaylistButton setEnabled:NO];
			[moveTracksUpButton setEnabled:NO];
			[moveTracksDownButton setEnabled:NO];
		}
		else if ([theNJB isConnected])
		{
			[deleteFromPlaylistButton setEnabled:YES];
			[moveTracksUpButton setEnabled:YES];
			[moveTracksDownButton setEnabled:YES];
		}
	}
}

/* shift the selected tracks up in order,
 * if possible
 */
- (IBAction)moveTracksUp:(id)sender
{
	Playlist *playlist = (Playlist *)[playlists objectAtIndex:[playlistsTable selectedRow]];
	
	// nothing to do if all selected
	if ([playlistsTrackTable numberOfSelectedRows] == [playlist trackCount])
		return;
	
	[playlist lock];
	
	NSEnumerator *enumerator = [playlistsTrackTable selectedRowEnumerator];
	NSNumber *currentIndex = nil;
	unsigned previousIndex = 0;
	BOOL selectionIncludesFirstElement = NO;
	unsigned index = 0;
	while (currentIndex = [enumerator nextObject])
	{
		index = [currentIndex unsignedIntValue];
		if (index == 0 || (selectionIncludesFirstElement && previousIndex == index - 1))
		{
			selectionIncludesFirstElement = YES;
			previousIndex = index;
			continue;
		}
		else
			selectionIncludesFirstElement = NO;

		[self swapPlaylistTrackWithPreviousAtIndex:index forPlaylist:playlist];
	}
	
	// scroll to upper most selected item
	// (which is now in new position)
	enumerator = [playlistsTrackTable selectedRowEnumerator];
	currentIndex = [enumerator nextObject];
	[playlistsTrackTable scrollRowToVisible:[currentIndex unsignedIntValue]];
	
	// only update if something has changed
	if (!selectionIncludesFirstElement)
		[self updateJukeboxPlaylist:playlist];
	
	[playlist unlock];
	
	[playlistsTrackTable reloadData];
}

/* shift the selected down up in order,
 * if possible
 */
- (IBAction)moveTracksDown:(id)sender
{
	Playlist *playlist = (Playlist *)[playlists objectAtIndex:[playlistsTable selectedRow]];
	
	// nothing to do if all selected
	if ([playlistsTrackTable numberOfSelectedRows] == [playlist trackCount])
		return;
	
	[playlist lock];
		
	// need to get indexes in reverse order (highest first)
	NSEnumerator *enumerator = [playlistsTrackTable selectedRowEnumerator];
	NSNumber *currentIndex = nil;
	NSMutableArray *indexes = [[NSMutableArray alloc] initWithCapacity:[playlistsTrackTable numberOfSelectedRows]];
	while (currentIndex = [enumerator nextObject])
		[indexes addObject:currentIndex];
	
	enumerator = [indexes reverseObjectEnumerator];
	unsigned previousIndex = 0;
	BOOL selectionIncludesLastElement = NO;
	while (currentIndex = [enumerator nextObject])
	{
		unsigned index = [currentIndex unsignedIntValue];
		if (index == [playlist trackCount] - 1 || (selectionIncludesLastElement && previousIndex == index + 1))
		{
			selectionIncludesLastElement = YES;
			previousIndex = index;
			continue;
		}
		else
			selectionIncludesLastElement = NO;
		
		[self swapPlaylistTrackWithNextAtIndex:index forPlaylist:playlist];
	}
	
	// scroll to lower most selected item
	// which needs to be adjusted to new position
	enumerator = [indexes reverseObjectEnumerator];
	currentIndex = [enumerator nextObject];
	unsigned selectIndex = [currentIndex unsignedIntValue];
	if (selectIndex != [playlist trackCount] - 1)
		selectIndex++;
	[playlistsTrackTable scrollRowToVisible:selectIndex];
	
	// only update if something has changed
	if (!selectionIncludesLastElement)
	[self updateJukeboxPlaylist:playlist];
	
	[playlist unlock];
	
	[playlistsTrackTable reloadData];
	[indexes release];
}

/* swaps the track at index with the track at index-1
 */
- (void)swapPlaylistTrackWithPreviousAtIndex:(unsigned)index forPlaylist:(Playlist *)playlist
{
	[playlist swapTrackWithPreviousAtIndex:index];
	[playlistsTrackTable deselectRow:index];
	[playlistsTrackTable selectRow:(index - 1) byExtendingSelection:YES];
}

/* swaps the track at index with the track at index+1
*/
- (void)swapPlaylistTrackWithNextAtIndex:(unsigned)index forPlaylist:(Playlist *)playlist
{
	[playlist swapTrackWithNextAtIndex:index];
	[playlistsTrackTable deselectRow:index];
	[playlistsTrackTable selectRow:(index + 1) byExtendingSelection:YES];
}

- (void)NJBTrackListModified:(NSNotification *)note
{
	trackListUpToDate = NO;
}

@end
