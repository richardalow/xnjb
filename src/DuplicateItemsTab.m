//
//  DuplicateItemsTab.m
//  XNJB
//
//  Created by Richard Low on 20/02/2005.
//

#import "DuplicateItemsTab.h"
#import "DuplicateTrackFinder.h"
#import "Preferences.h"
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
#define COLUMN_RATING @"Rating"

@implementation DuplicateItemsTab

- (void)dealloc
{
	[itemArray release];
	[duplicatesArray release];
	[continueFindingDuplicateTracks release];
	[super dealloc];
}

/*
 * fill the njbTable
 */
- (id)tableView:(NSTableView *)aTableView
   objectValueForTableColumn:(NSTableColumn *)aTableColumn
						row:(int)rowIndex
{	
	NSString *ident = [aTableColumn identifier];
	Track *track = (Track *)[duplicatesArray objectAtIndex:rowIndex];
	if ([ident isEqual:COLUMN_TITLE])
		return [track title];
	else if ([ident isEqual:COLUMN_ALBUM])
		return [track album];
	else if ([ident isEqual:COLUMN_ARTIST])
		return [track artist];
	else if ([ident isEqual:COLUMN_GENRE])
		return [track genre];
	else if ([ident isEqual:COLUMN_LENGTH])
		return [track lengthString];
	else if ([ident isEqual:COLUMN_TRACK_NO])
		return [NSString stringWithFormat:@"%d", [track trackNumber]];
	else if ([ident isEqual:COLUMN_CODEC])
		return [track fileTypeString];
	else if ([ident isEqual:COLUMN_FILESIZE])
		return [track filesizeString];
	else if ([ident isEqual:COLUMN_YEAR])
		return [NSString stringWithFormat:@"%d", [track year]];
  else if ([ident isEqual:COLUMN_RATING])
    return [NSNumber numberWithUnsignedInt:[track rating]];
	else
	{
		NSLog(@"Invalid column tag in DuplicateItemsTab");
		return nil;
	}
}

- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
	if (duplicatesArray == nil)
		return 0;
	else
		return [duplicatesArray count];
}

- (void)onConnectAndActive
{
	[super onConnectAndActive];
	[self loadTracks];
}

- (void)NJBConnected:(NSNotification *)note
{
	trackListUpToDate = NO;
	
	[findDuplicatesButton setEnabled:YES];
	[super NJBConnected:note];
}

- (void)NJBDisconnected:(NSNotification *)note
{
	trackListUpToDate = NO;
	
	[findDuplicatesButton setEnabled:NO];
	[deleteButton setEnabled:NO];
	[super NJBDisconnected:note];
}

- (void)activate
{
	// this must be called first to run onConnectAndActive so we can download the tracks there
	[super activate];
	if (!trackListUpToDate && [theNJB isConnected])
	{
		[duplicatesArray release];
		duplicatesArray = nil;
		[njbTable reloadData];
		[self loadTracks];
	}
	else
	{
		if ([njbTable selectedRow] != -1)
		{
			[drawerController showTrack:(Track *)[duplicatesArray objectAtIndex:[njbTable selectedRow]]];
			if (![theNJB isConnected])
				[drawerController disableWrite];
			else
				[deleteButton setEnabled:YES];
		}
	}
}

/* add downloadTrackList to the njb queue
*/
- (void)loadTracks
{	
	if (trackListUpToDate)
		return;
	
	// give up any duplicate finding
	[continueFindingDuplicateTracks setValue:NO];
	
	NSMutableArray *cachedTrackList = [theNJB cachedTrackList];
	if (cachedTrackList != nil)
	{
		[itemArray release];
		itemArray = cachedTrackList;
		[itemArray retain];
		[self findDuplicates];
		
		trackListUpToDate = YES;
		return;
	}
	
	NJBQueueItem *getTracks = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(downloadTrackList)];
	[getTracks setStatus:STATUS_DOWNLOADING_TRACK_LIST];
	
	NJBQueueItem *updateTable = [[NJBQueueItem alloc] initWithTarget:self
																											withSelector:@selector(findDuplicates)
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
	[itemArray release];
	itemArray = [theNJB tracks];
	
	NJBTransactionResult *result = [[NJBTransactionResult alloc] initWithSuccess:(itemArray != nil)];
	
	[itemArray retain];
	
	trackListUpToDate = YES;
	
	return [result autorelease];
}

- (void)NJBTrackListModified:(NSNotification *)note
{
	if (!isActive)
		trackListUpToDate = NO;
	[continueFindingDuplicateTracks setValue:NO];
	[continueFindingDuplicateTracks release];
	continueFindingDuplicateTracks = nil;
}

- (void)findDuplicates
{
	[duplicatesArray release];
	duplicatesArray = nil;
	[njbTable reloadData];
	[findDuplicatesButton setEnabled:NO];
	[findDuplicatesStatusField setStringValue:NSLocalizedString(@"Finding Duplicates...", nil)];
	// check we have cancelled any other duplicate finding threads
	[continueFindingDuplicateTracks setValue:NO];
	[continueFindingDuplicateTracks release];
	continueFindingDuplicateTracks = [[MyBool alloc] initWithValue:YES];
	// retain it twice, is released when worker thread done and when we get here again
	[continueFindingDuplicateTracks retain];
	
	[NSThread detachNewThreadSelector:@selector(findDuplicatesWorker:) toTarget:self withObject:[NSArray arrayWithObjects:itemArray, continueFindingDuplicateTracks, nil]];
	// we don't need it anymore, he will free it
	itemArray = nil;
}

- (void)findDuplicatesWorker:(NSArray *)items
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	if ([items count] != 2)
	{
		NSLog(@"object array count != 2 in findDuplicatesWorker!!");
		return;
	}
	NSMutableArray *tracks = [items objectAtIndex:0];
	MyBool *carryOn = [items objectAtIndex:1];
	
	duplicatesArray = [[DuplicateTrackFinder findDuplicates:tracks
																									lengthTol:[preferences lengthTol]
																								filesizeTol:[preferences filesizeTol]*100000
																									 titleTol:[preferences titleTol]
																									artistTol:[preferences artistTol]
																									  carryOn:carryOn] retain];
	// test to save time processing...
	//duplicatesArray = [[MyMutableArray alloc] initWithMyMutableArray:itemArray];
	
	NJBQueueItem *updateTable = [[NJBQueueItem alloc] initWithTarget:njbTable
																											withSelector:@selector(reloadData)
																												withObject:nil
																							 withRunInMainThread:YES];
	[updateTable setDisplayStatus:NO];
	
	NJBQueueItem *enableFindDuplicatesButton = [[NJBQueueItem alloc] initWithTarget:self
																																		 withSelector:@selector(enableFindDuplicatesButton)
																																			 withObject:nil
																															withRunInMainThread:YES];
	[enableFindDuplicatesButton setDisplayStatus:NO];

	// this adds to the bottom of the queue - noone is allowed to start me again until all transfers finished
	[self addToQueue:updateTable withSubItems:[NSArray arrayWithObjects:enableFindDuplicatesButton, nil] withDescription:@""];
	[updateTable release];
	[enableFindDuplicatesButton release];
	
	// was retained for us, we're done now so can release
	[tracks release];
	[carryOn release];
	
	[pool release];
}

- (IBAction)findDuplicatesButton:(id)sender;
{
	// force reload
	trackListUpToDate = NO;
	[self loadTracks];
}

- (void)enableFindDuplicatesButton
{
	[findDuplicatesButton setEnabled:YES];
	[findDuplicatesStatusField setStringValue:@""];
}

- (void)deleteFromNJB:(Track *)track
{	
	NJBQueueItem *deleteTrack = [[NJBQueueItem alloc] initWithTarget:theNJB withSelector:@selector(deleteTrack:)
																												withObject:track];
	[deleteTrack setStatus:STATUS_DELETING_TRACK];
		
	NJBQueueItem *removeFromItemArray = [[NJBQueueItem alloc] initWithTarget:itemArray withSelector:@selector(removeObject:)
																																withObject:track];
	NJBQueueItem *removeFromDuplicatesArray = [[NJBQueueItem alloc] initWithTarget:duplicatesArray withSelector:@selector(removeObject:)
																																			withObject:track];
	[removeFromItemArray setRunInMainThread:YES];
	[removeFromItemArray setDisplayStatus:NO];
	[removeFromDuplicatesArray setRunInMainThread:YES];
	[removeFromDuplicatesArray setDisplayStatus:NO];

	NJBQueueItem *updateTable = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(reloadData)];
	[updateTable setRunInMainThread:YES];
	[updateTable setDisplayStatus:NO];
	
	[deleteTrack cancelItemIfFail:removeFromItemArray];
	[deleteTrack cancelItemIfFail:removeFromDuplicatesArray];
	[deleteTrack cancelItemIfFail:updateTable];
	
	NSString *description = [NSString stringWithFormat:@"%@ '%@'", NSLocalizedString(@"Deleting track", nil), [track title]];
	[self addToQueue:deleteTrack withSubItems:[NSArray arrayWithObjects:removeFromItemArray, removeFromDuplicatesArray, updateTable, nil] withDescription:description];
	
	[deleteTrack release];
	[removeFromItemArray release];
	[removeFromDuplicatesArray release];
	[updateTable release];
}

/* reloads the njbTable data after deletion
 * and updates the tag drawer
 */
- (void)reloadData
{
	[njbTable reloadData];
	
	if (isActive)
	{
		int newRow = [njbTable selectedRow];
		if (newRow != -1)
		{
			// update display to show new track
			[drawerController showTrack:(Track *)[duplicatesArray objectAtIndex:newRow]];
			if (![theNJB isConnected])
				[drawerController disableWrite];
		}
		else
		{
			[drawerController clearAll];
			[drawerController disableAll];
		}
	}
}

/* update the drawer when someone selects a different
* track
*/
- (void)tableViewSelectionDidChange:(NSNotification *)aNotification
{
	// unchanged tag edit will be lost
	
	int newRow = [njbTable selectedRow];
	if (newRow == -1)
	{
		[deleteButton setEnabled:NO];
		[drawerController clearAll];
		[drawerController disableAll];
		return;
	}
	
	// update display to show new track
	[drawerController showTrack:(Track *)[duplicatesArray objectAtIndex:newRow]];
	if (![theNJB isConnected])
		[drawerController disableWrite];
	
	[deleteButton setEnabled:YES];
}

/* called by MainController when we must write
* a track to a file/njb
*/
- (void)drawerWriteTag:(Track *)modifiedTrack
{
	int rowIndex = [njbTable selectedRow];
	// none selected, shouldn't be here anyway
	if (rowIndex == -1)
		return;	
		
	Track *oldTrack = (Track *)[duplicatesArray objectAtIndex:rowIndex];
	
	NJBQueueItem *changeTrackTag = [[NJBQueueItem alloc] initWithTarget:theNJB withSelector:@selector(changeTrackTagTo:from:) withObject:modifiedTrack];
	[changeTrackTag setObject2:oldTrack];
	[changeTrackTag setStatus:STATUS_UPDATING_TRACK_TAG];
			
	NJBQueueItem *updateTrackList = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(replaceTrack:withTrack:)
																														withObject:oldTrack withRunInMainThread:YES];
	[updateTrackList setObject2:modifiedTrack];
	[updateTrackList setDisplayStatus:NO];
			
	NJBQueueItem *updateDisplay = [[NJBQueueItem alloc] initWithTarget:njbTable withSelector:@selector(reloadData)];
	[updateDisplay setRunInMainThread:YES];
	[updateDisplay setDisplayStatus:NO];
	
	[changeTrackTag cancelItemIfFail:updateTrackList];
	[changeTrackTag cancelItemIfFail:updateDisplay];
			
	NSString *description = [NSString stringWithFormat:@"%@ '%@'", NSLocalizedString(@"Changing tag for track", nil), [oldTrack title]];
	[self addToQueue:changeTrackTag withSubItems:[NSArray arrayWithObjects:updateTrackList, updateDisplay, nil] withDescription:description];
			
	[changeTrackTag release];
	[updateTrackList release];
	[updateDisplay release];
}

- (void)replaceTrack:(Track *)replace withTrack:(Track *)new
{
	[itemArray replaceObject:replace withObject:new];
	[duplicatesArray replaceObject:replace withObject:new];
}

/* delete all the selected items off the NJB, after warning
*/
- (void)deleteFromNJB
{
	int result = NSRunAlertPanel(NSLocalizedString(@"File Deletion", nil),
															 NSLocalizedString(@"Are you sure you want to delete the selected items off the device?", nil),
															 NSLocalizedString(@"No", nil),
															 NSLocalizedString(@"Yes", nil), nil);
	if (result == NSAlertDefaultReturn)
		return;
	
	/* build up an array of items to be deleted
		* we can't pass indexes as they aren't valid
		* after items have been deleted
		*/
	NSEnumerator *enumerator = [njbTable selectedRowEnumerator];
	NSNumber *index;
	NSMutableArray *selectedItems = [[NSMutableArray alloc] initWithCapacity:[njbTable numberOfSelectedRows]];
	while (index = [enumerator nextObject])
	{
	  [selectedItems addObject:[duplicatesArray objectAtIndex:[index intValue]]];
	}
	enumerator = [selectedItems objectEnumerator];
	MyItem *currentItem;
	while (currentItem = [enumerator nextObject])
	{
		[self deleteFromNJB:(Track *)currentItem];
	}
	[selectedItems release];
}

- (IBAction)deleteButton:(id)sender
{
	[self deleteFromNJB];
}

- (BOOL)menuShouldDelete
{
	if (![theNJB isConnected])
		return NO;
	if ([njbTable selectedRow] != -1)
		return YES;
	else
		return NO;
}

- (void)menuDelete
{
	[self deleteFromNJB];
}

@end
