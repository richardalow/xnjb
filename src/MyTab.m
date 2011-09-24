//
//  MyTab.m
//  XNJB
//
//  Created by Richard Low on Fri Jul 16 2004.
//

/* 

The tab class hierarchy:

      MyTab -> MyTableTab -> MyFileTab
        |           |            |
     QueueTab PlaylistsTab    DataTab
        |                        |
   SettingsTab								MusicTab
        |
DuplicateItemsTab

MyTab
-----

This is the base class for all the tabs.
Defines activate/deactivate, onConnectAndActive and addToQueue.

MyTableTab
----------

This class is for a tab which has an NSTableView used to show items on the
NJB e.g. tracks or datafiles. It allows sorting and searching of the list.

MyFileTab
---------

This is for a tab that has both an NSTableView for jukebox items and a FileSystemBrowser 
for local items. Implements methods for copying between the two and deleting.

*/

#import "MyTab.h"
#import "defs.h"

@implementation MyTab

// init/dealloc methods

- (id)init
{
	if (self = [super init])
	{
		isActive = NO;
		hasActivatedSinceConnect = NO;
		
		NSNotificationCenter *nc;
		nc = [NSNotificationCenter defaultCenter];
		[nc addObserver:self selector:@selector(NJBConnected:) name:NOTIFICATION_CONNECTED object:nil];
		[nc addObserver:self selector:@selector(NJBDisconnected:) name:NOTIFICATION_DISCONNECTED object:nil];
		[nc addObserver:self selector:@selector(NJBTrackListModified:) name:NOTIFICATION_TRACK_LIST_MODIFIED object:nil];
		
		// in case we're the queue tab
		if (queueTab == nil)
			queueTab = self;
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[njbQueue release];
	[super dealloc];
}

// accessor methods

- (void)setNJBQueueConsumer:(NJBQueueConsumer *)newNjbQueue
{
	[newNjbQueue retain];
	[njbQueue release];
	njbQueue = newNjbQueue;
}

/**********************/

- (void)awakeFromNib
{
	// disable items as we can't be connected yet
	[self NJBDisconnected:nil];
}

/* called by main controller when this tab
 * is activated
 */
- (void)activate
{
	isActive = YES;
	if (!hasActivatedSinceConnect && [theNJB isConnected])
		[self NJBConnected:nil];
}

/* called by main controller when this tab
 * is deactivated
 */
- (void)deactivate
{
	isActive = NO;
}

/* methods to add NJBQueueItems to the queue
 * and to queueTab's list
 */
- (void)addToQueue:(NJBQueueItem *)item
{
	[self addToQueue:item withSubItems:nil withDescription:nil];
}

- (void)addToQueue:(NJBQueueItem *)item withSubItems:(NSArray *)subItems
{
	[self addToQueue:item withSubItems:subItems withDescription:nil];
}

- (void)addToQueue:(NJBQueueItem *)item withSubItems:(NSArray *)subItems withDescription:(NSString *)description
{
	[[njbQueue lock] lock];
	[[njbQueue queue] addObject:item];
	
	NSEnumerator *enumerator = [subItems objectEnumerator];
	NJBQueueItem *currentItem;
	while (currentItem = [enumerator nextObject])
	{
		[[njbQueue queue] addObject:currentItem];
	}
	
	[[njbQueue lock] unlockWithCondition:([[njbQueue queue] isEmpty] ? NO_QUEUE_ITEMS : SOME_QUEUE_ITEMS)];
	
	if ([item displayStatus])
		[queueTab addItem:item withSubItems:subItems withDescription:description];
}

- (void)NJBConnected:(NSNotification *)note
{
	if (isActive)
	{
		hasActivatedSinceConnect = YES;
		[self onConnectAndActive];
	}
}

- (void)NJBDisconnected:(NSNotification *)note
{
	hasActivatedSinceConnect = NO;
}

/* to be implemented by superclass:
 * called when we we are activated for the
 * first time since connect (including if
 * we are already active on connect)
 */
- (void)onConnectAndActive{}

- (void)NJBTrackListModified:(NSNotification *)note{}

@end
