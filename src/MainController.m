//
//  MainController.m
//  XNJB
//

#import "MainController.h"
#import "NJBQueueItem.h"
#import "defs.h"

@implementation MainController

// init/dealloc methods

- (id)init
{
	if (self = [super init])
	{
		myQueue = [[Queue alloc] init];
		myQueueLock = [[NSLock alloc] init];
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[njbQueue release];
	[kitConnection release];
	[super dealloc];
}

// accessor methods

- (DataTab *)dataTab
{
	return dataTab;
}

- (MusicTab *)musicTab
{
	return musicTab;
}

- (PlaylistsTab *)playlistsTab
{
	return playlistsTab;
}

- (QueueTab *)queueTab
{
	return queueTab;
}

- (SettingsTab *)settingsTab
{
	return settingsTab;
}

- (DuplicateItemsTab *)duplicateItemsTab
{
	return duplicateItemsTab;
}

- (InfoTab *)infoTab
{
	return infoTab;
}

/**********************/

- (void)awakeFromNib
{
	// tell drawerController to send messages here when write tag button is clicked
	[drawerController setWriteTagSelector:@selector(drawerWriteTag:)
																 target:self];
	
	[windowMain setFirstResponderChangeTarget:self];
	[windowMain setFirstResponderChangeSelector:@selector(onFirstResponderChange:)];
	
	[self initNJB];
	
	[NSApp setDelegate:self];
	
	/* Select tab index [preferences startupTab] and send the activate only once.
	 * If currentIndex == [preferences startupTab] and don't call tabActivate manually, no activate sent.
	 * When current tab changes, didSelectTabViewItem is called anyway
	 */
	if ([self currentTabIndex] == [preferences startupTab])
	{
		[[self tabAtIndex:[preferences startupTab]] activate];
	}
	else
	{
		[tabViewMain selectTabViewItemAtIndex:[preferences startupTab]];
	}
	
	[status setMainThreadQueueLock:myQueueLock];
	
	NSNotificationCenter *nc;
	nc = [NSNotificationCenter defaultCenter];
	[nc addObserver:self selector:@selector(NJBConnected:) name:NOTIFICATION_CONNECTED object:nil];
	[nc addObserver:self selector:@selector(NJBDisconnected:) name:NOTIFICATION_DISCONNECTED object:nil];
}

- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
	int newIndex = [tabView indexOfTabViewItem:tabViewItem];
	[[self tabAtIndex:previousTabIndex] deactivate];
	previousTabIndex = newIndex;
	[drawerController clearAll];
	[drawerController disableAll];
	[[self tabAtIndex:newIndex] activate];
}

- (void)drawerWriteTag:(Track *)modifiedTrack
{
	int tabIndex = [tabViewMain indexOfTabViewItem:[tabViewMain selectedTabViewItem]];
	if (tabIndex == TAB_MUSIC)
		[musicTab drawerWriteTag:modifiedTrack];
	else if (tabIndex == TAB_DUPLICATES)
		[duplicateItemsTab drawerWriteTag:modifiedTrack];
}

- (void)onFirstResponderChange:(NSResponder *)aResponder
{
	int tabIndex = [tabViewMain indexOfTabViewItem:[tabViewMain selectedTabViewItem]];
	switch (tabIndex)
  {
		case TAB_MUSIC:
			[musicTab onFirstResponderChange:aResponder];
			break;
		case TAB_DATA:
			[dataTab onFirstResponderChange:aResponder];
			break;
  }
}

- (void)initNJB
{
	[theNJB setTurbo:[preferences turbo]];
  [theNJB setCreateAlbumFiles:[preferences createAlbumFiles]];
  if ([preferences createAlbumFiles])
  {
    [theNJB setUploadAlbumArt:[preferences uploadAlbumArt]];
    if ([preferences uploadAlbumArt])
    {
      [theNJB setAlbumArtWidth:[preferences albumArtWidth]];
      [theNJB setAlbumArtHeight:[preferences albumArtHeight]];
    }
  }
	
	// set up NSConnection between threads so the queue can call us 
	NSPort *port1;
	NSPort *port2;
	NSArray *portArray;
	
	port1 = [NSPort port];
	port2 = [NSPort port];
	kitConnection = [[NSConnection alloc] initWithReceivePort:port1 sendPort:port2];
	[kitConnection setRootObject:self];
		
	// Ports switched here
	portArray = [NSArray arrayWithObjects:port2, port1, nil];
		
	Queue *queue = [[Queue alloc] init];
	NSConditionLock *lock = [[NSConditionLock alloc] init];
	njbQueue = [[NJBQueueConsumer alloc] initWithQueue:queue withLock:lock];
	
	[njbQueue setStatusDisplayer:status];
	[njbQueue setMainThreadQueue:myQueue];
	[njbQueue setMainThreadQueueLock:myQueueLock];
	[status setMainThreadQueue:myQueue];
	
	[njbQueue consume];
	
	NJBQueueItem *initConnection = [[NJBQueueItem alloc] initWithTarget:njbQueue withSelector:@selector(connectToMainThreadWithPorts:) withObject:portArray];
	[initConnection setDisplayStatus:NO];
	
	[lock lock];
	[queue addObject:initConnection];
	[lock unlockWithCondition:([queue isEmpty] ? NO_QUEUE_ITEMS : SOME_QUEUE_ITEMS)];
	
	[initConnection release];
	[queue release];
	[lock release];
	
	if ([preferences connectOnStartup])
		[self connectToNJB];
    
  // TODO: set pref to see if want to connect on device added
////  [NSThread detachNewThreadSelector:@selector(runNotificationLoopWithObject:) toTarget:theNJB withObject:[NSArray arrayWithObjects:queueTab, njbQueue, nil]];
	
	[musicTab setNJBQueueConsumer:njbQueue];
	[queueTab setNJBQueueConsumer:njbQueue];
	[dataTab setNJBQueueConsumer:njbQueue];
	[playlistsTab setNJBQueueConsumer:njbQueue];
	[duplicateItemsTab setNJBQueueConsumer:njbQueue];
	[settingsTab setNJBQueueConsumer:njbQueue];
	[infoTab setNJBQueueConsumer:njbQueue];
  
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
	if (![self shouldClose])
	{
		return NSTerminateCancel;
	}
	// do it in this thread as want to block
	[theNJB disconnect];
	
	[preferences setLastUsedTab:[self currentTabIndex]];
	
	[self postNotificationName:NOTIFICATION_APPLICATION_TERMINATING object:nil];
	
	return NSTerminateNow;
}

- (BOOL)shouldClose
{
	if (![status isIdle])
	{
		int result = NSRunAlertPanel(NSLocalizedString(@"Quit XNJB", nil),
																 NSLocalizedString(@"There are still items in the queue.  Are you sure you want to quit?  This could crash your NJB and/or XNJB.", nil),
																 NSLocalizedString(@"No", nil),
																 NSLocalizedString(@"Yes", nil), nil);
		if (result == NSAlertDefaultReturn)
			return NO;
	}
	return YES;
}

- (id)currentTab
{
	return [self tabAtIndex:[tabViewMain indexOfTabViewItem:[tabViewMain selectedTabViewItem]]];
}

- (int)currentTabIndex
{
	return [tabViewMain indexOfTabViewItem:[tabViewMain selectedTabViewItem]];
}

- (id)tabAtIndex:(unsigned)index
{
	switch (index)
	{
		case TAB_PLAYLISTS:
			return playlistsTab;
			break;
		case TAB_MUSIC:
			return musicTab;
			break;
		case TAB_DATA:
			return dataTab;
			break;
		case TAB_SETTINGS:
			return settingsTab;
			break;
		case TAB_DUPLICATES:
			return duplicateItemsTab;
			break;
		case TAB_INFO:
			return infoTab;
			break;
		case TAB_QUEUE:
			return queueTab;
			break;
		default:
			NSLog(@"Invalid tab index");
			return nil;
	}
}

/* I have a queue that StatusController adds to to update
 * UI components, that can't be done in the worker thread.
 * When something has been added, consumeQueue should be
 * called so we know to process
 */
- (void)consumeQueue
{
	NJBQueueItem *currentItem;
	[myQueueLock lock];
	while (currentItem = [myQueue nextObject])
	{
		id result = [currentItem process];
		
		if ([currentItem displayStatus])
		{
			if ([result class] == [NJBTransactionResult class])
				[status taskComplete:result];
			else
				[status taskComplete:[[NJBTransactionResult alloc] initWithSuccess:YES]];
		}
	}
	[myQueueLock unlock];
}

// remember object is a proxy to the real object if this is called through mainThreadProxy in StatusDisplayer
- (void)postNotificationName:(NSString *)name object:(id)object
{
	NSNotificationCenter *nc;
	nc = [NSNotificationCenter defaultCenter];
	[nc postNotificationName:name object:object];
}

- (void)connectToNJB
{
	NJBQueueItem *connect = [[NJBQueueItem alloc] initWithTarget:theNJB withSelector:@selector(connect)];
	[connect setStatus:STATUS_CONNECTING];
	
	[queueTab addItem:connect withSubItems:nil withDescription:NSLocalizedString(@"Connecting to NJB", nil)];
	
	[[njbQueue lock] lock];
	[[njbQueue queue] addObject:connect];
	[[njbQueue lock] unlockWithCondition:([[njbQueue queue] isEmpty] ? NO_QUEUE_ITEMS : SOME_QUEUE_ITEMS)];
  
  [connect release];
}

- (void)disconnectFromNJB
{
	NJBQueueItem *disconnect = [[NJBQueueItem alloc] initWithTarget:theNJB withSelector:@selector(disconnect)];
	[disconnect setStatus:STATUS_DISCONNECTING];
	[queueTab addItem:disconnect withSubItems:nil withDescription:NSLocalizedString(@"Disconnecting from NJB", nil)];
	
	[[njbQueue lock] lock];
	[[njbQueue queue] addObject:disconnect];
	[[njbQueue lock] unlockWithCondition:([[njbQueue queue] isEmpty] ? NO_QUEUE_ITEMS : SOME_QUEUE_ITEMS)];
  
  [disconnect release];
}

// menu methods

- (BOOL)menuShouldConnect
{
	return YES;
}

- (BOOL)menuShouldDisconnect
{
	return [theNJB isConnected];
}

- (void)menuConnect
{
	if (![theNJB isConnected])
	{
		[self connectToNJB];
		return;
	}
	// ask if they really want to try to connect if appears already connected
	// we don't get told when they pull the plug out so maybe we're not, let's ask...
	int result = NSRunAlertPanel(NSLocalizedString(@"Connect to NJB", nil),
															 NSLocalizedString(@"It appears that we are already connected. Do you want to try to connect again?", nil),
															 NSLocalizedString(@"No", nil),
															 NSLocalizedString(@"Yes", nil), nil);
	if (result == NSAlertDefaultReturn)
		return;
	[self connectToNJB];
}

- (void)menuDisconnect
{
	[self disconnectFromNJB];
}

- (IBAction)buttonConnect:(id)sender
{
	if (![theNJB isConnected])
		[self connectToNJB];
	else
		[self disconnectFromNJB];
}

/* called when we disconnect
*/
- (void)NJBDisconnected:(NSNotification *)note
{
	[connectButton setTitle:NSLocalizedString(@"Connect", nil)];
}

/* called when we connect
*/
- (void)NJBConnected:(NSNotification *)note
{
	[connectButton setTitle:NSLocalizedString(@"Disconnect", nil)];
}

/* called when the main window is closing
 */
- (BOOL)windowShouldClose:(id)sender
{
	[NSApp terminate:sender];
	// don't close the window, because if we said don't close the app, we don't want the window closed but app still running
	// if user asked to terminate app rather than just close window, windowShouldClose: is not called so we will close anyway
	return NO;
}

@end
