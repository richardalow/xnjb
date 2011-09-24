//
//  QueueTab.m
//  XNJB
//

/* this is the queue tab to show what is in the
 * njb queue. Allows users to cancel items.
 */
#import "QueueTab.h"
#import "QueueTabItem.h"
#import "defs.h"

// these correspond to the identifiers in the NIB file
#define COLUMN_TYPE @"Type"
#define COLUMN_DESCRIPTION @"Description"
#define COLUMN_STATUS @"Status"
#define COLUMN_RESULT @"Result"

@implementation QueueTab

// init/dealloc methods

- (id)init
{
	if (self = [super init])
	{
		queueItems = [[NSMutableArray alloc] init];
		currentTaskIndex = -1;
		
		imageQueued = [[NSImage imageNamed:@"queued"] retain];
		imageRunning = [[NSImage imageNamed:@"running"] retain];
		imageSuccess = [[NSImage imageNamed:@"success"] retain];
		imageFailure = [[NSImage imageNamed:@"failure"] retain];
		imageCancelled = [[NSImage imageNamed:@"cancelled"] retain];
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[queueItems release];
	
	[imageQueued release];
	[imageRunning release];
	[imageSuccess release];
	[imageFailure release];
	[imageCancelled release];
	
	[super dealloc];
}

/**********************/

- (void)activate
{
	[queueTable reloadData];
}

- (void)awakeFromNib
{	
	NSNotificationCenter *nc;
	nc = [NSNotificationCenter defaultCenter];
	[nc addObserver:self selector:@selector(taskComplete:) name:NOTIFICATION_TASK_COMPLETE object:nil];
	[nc addObserver:self selector:@selector(taskStarting:) name:NOTIFICATION_TASK_STARTING object:nil];
}

/* should be called when an item is added to the 
 * njb queue if it should be shown & cancelable
 */
- (void)addItem:(NJBQueueItem *)mainItem withSubItems:(NSArray *)subItems withDescription:(NSString *)description
{
	QueueTabItem *newItem = [[QueueTabItem alloc] initWithMainItem:mainItem withSubItems:subItems];
	[newItem setDescription:description];
	[queueItems addObject:newItem];
	[newItem release];
	
	[statusDisplayer incrementTaskCount];
	
	[queueTable reloadData];
}

- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
	return [queueItems count];
}

- (id)tableView:(NSTableView *)aTableView
   objectValueForTableColumn:(NSTableColumn *)aTableColumn
						row:(int)rowIndex
{	
	NSString *ident = [aTableColumn identifier];
	itemStatusTypes itemStatus;
	if ([ident isEqual:COLUMN_TYPE])
		return [statusDisplayer statusStringForStatus:[[[queueItems objectAtIndex:rowIndex] mainItem] status]];
	else if ([ident isEqual:COLUMN_DESCRIPTION])
		return [[queueItems objectAtIndex:rowIndex] description];
	else if ([ident isEqual:COLUMN_STATUS])
	{
		itemStatus = [[queueItems objectAtIndex:rowIndex] status];
		NSImage *image = nil;
		switch (itemStatus)
		{
		case ITEM_STATUS_QUEUED:
				image = imageQueued;
				break;
			case ITEM_STATUS_RUNNING:
				image = imageRunning;
				break;
			case ITEM_STATUS_FINISHED_WITH_SUCCESS:
				image = imageSuccess;
				break;
			case ITEM_STATUS_FINISHED_WITH_FAILURE:
				image = imageFailure;
				break;
			case ITEM_STATUS_CANCELLED:
				image = imageCancelled;
				break;
		}
			
		NSMutableAttributedString* nstr = nil; 
		NSTextAttachmentCell* ntac = nil; 
		NSTextAttachment* ntat = nil; 
		NSAttributedString* spac = nil; 
		NSAttributedString* text = nil; 
		
		ntac = [[[NSTextAttachmentCell alloc] init] autorelease]; 
		[ntac setImage:image]; 
		
		ntat = [[[NSTextAttachment alloc] init] autorelease]; 
		[ntat setAttachmentCell:ntac]; 
		
		spac = [[NSAttributedString alloc] initWithString:@" "]; 
		text = [[NSAttributedString alloc] initWithString:[self statusStringForStatus:itemStatus]]; 
		
		[spac autorelease]; 
		[text autorelease]; 
		
		nstr = (NSMutableAttributedString*)[NSMutableAttributedString attributedStringWithAttachment:ntat];
		
		[nstr appendAttributedString:spac]; 
		[nstr appendAttributedString:text]; 
		
		return nstr; 
  }
	else if ([ident isEqual:COLUMN_RESULT])
		return [[queueItems objectAtIndex:rowIndex] resultString];
	else
	{
		NSLog(@"Invalid column tag in QueueTab");
		return nil;	
	}
}

/* get a string for the status of an item
 */
- (NSString *)statusStringForStatus:(itemStatusTypes)status
{
	switch (status)
	{
		case ITEM_STATUS_QUEUED:
			return NSLocalizedString(@"Queued", nil);
		case ITEM_STATUS_RUNNING:
			return NSLocalizedString(@"Running", nil);
		case ITEM_STATUS_FINISHED_WITH_SUCCESS:
			return NSLocalizedString(@"Finished", nil);
		case ITEM_STATUS_FINISHED_WITH_FAILURE:
			return NSLocalizedString(@"Error", nil);
		case ITEM_STATUS_CANCELLED:
			return NSLocalizedString(@"Cancelled", nil);
		default:
			NSLog(@"Invalid status in QueueTab.statusStringForStatus");
			return @"Unknown";
	}
}

/* called when a task finishes
 */
- (void)taskComplete:(NSNotification *)note
{	
	if (currentTaskIndex >= [queueItems count])
	{
		NSLog(@"Error in QueueTab: currentTaskIndex is too big!");
		return;
	}
	
	QueueTabItem *currentItem = [queueItems objectAtIndex:currentTaskIndex];
	
	NJBTransactionResult *result = [note object];
	
	[currentItem setResultString:[result resultString]];
	
	if ([currentItem status] != ITEM_STATUS_CANCELLED)
	{
		if ([result success])
			[currentItem setStatus:ITEM_STATUS_FINISHED_WITH_SUCCESS];
		else
    {
			[currentItem setStatus:ITEM_STATUS_FINISHED_WITH_FAILURE];
      if ([[result extendedErrorString] length] > 0)
        [currentItem setResultString:[NSString stringWithFormat:@"%@ (%@)", [currentItem resultString], [result extendedErrorString]]];
    }
		[queueTable reloadData];
	}
}

/* called when a task starts
 */
- (void)taskStarting:(NSNotification *)note
{	
	currentTaskIndex++;
	while ([[queueItems objectAtIndex:currentTaskIndex] status] == ITEM_STATUS_CANCELLED)
	{
		currentTaskIndex++;
		if (currentTaskIndex >= [queueItems count])
		{
			NSLog(@"Error in QueueTab: currentTaskIndex is too big!");
			return;
		}
	}
		
	if (currentTaskIndex >= [queueItems count])
	{
		NSLog(@"Error in QueueTab: currentTaskIndex is too big!");
		return;
	}
	QueueTabItem *currentItem = [queueItems objectAtIndex:currentTaskIndex];
	if ([currentItem status] != ITEM_STATUS_CANCELLED)
	{
		[currentItem setStatus:ITEM_STATUS_RUNNING];
		[queueTable reloadData];
	}
}

/* can we cancel any of the selected items?
 */
- (BOOL)menuShouldCancelQueuedItem
{
	NSArray *itemsToCancel = [self selectedItemsToCancel];
	return ([itemsToCancel count] != 0);
}

/* cancel the selected items (if we can)
 */
- (void)menuCancelQueuedItem
{
	NSArray *itemsToCancel = [self selectedItemsToCancel];
	if ([itemsToCancel count] == 0)
	{
		// should not be here
		NSLog(@"itemsToCancel count is 0 in menuCancelQueuedItem!");
		return;
	}
	NSEnumerator *enumerator = [itemsToCancel objectEnumerator];
	QueueTabItem *queueItem;
	while (queueItem = [enumerator nextObject])
	{
    if ([queueItem status] == ITEM_STATUS_RUNNING)
      [theNJB cancelCurrentTransaction];
    [[njbQueue lock] lock];
		[queueItem cancel];
    [[njbQueue lock] unlockWithCondition:([[njbQueue queue] isEmpty] ? NO_QUEUE_ITEMS : SOME_QUEUE_ITEMS)];
	}
	[statusDisplayer cancelTasks:[itemsToCancel count]];
	[queueTable reloadData];
}

/* which of the selected items can we cancel?
 * Return selected items that are queued
 * (not cancelled, completed, etc.)
 */
- (NSArray *)selectedItemsToCancel
{
	NSMutableArray *itemsToCancel = [[NSMutableArray alloc] init];
	NSEnumerator *enumerator = [queueTable selectedRowEnumerator];
	QueueTabItem *queueItem;
	NSNumber *index;
	while (index = [enumerator nextObject])
	{
		queueItem = [queueItems objectAtIndex:[index unsignedIntValue]];
		
		if ([queueItem status] == ITEM_STATUS_QUEUED || [queueItem status] == ITEM_STATUS_RUNNING)
			[itemsToCancel addObject:queueItem];
	}
	NSArray *items = [NSArray arrayWithArray:itemsToCancel];
	[itemsToCancel release];
	return items;
}

@end
