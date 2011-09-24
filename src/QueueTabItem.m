//
//  QueueTabItem.m
//  XNJB
//
//  Created by Richard Low on Wed Jul 28 2004.
//

/* The items that are added to the queue list
 * Stores sub items so items other than ones being
 * shown can be cancelled
 */

#import "QueueTabItem.h"


@implementation QueueTabItem

// init/dealloc methods

- (id)init
{
	return [self initWithMainItem:nil withSubItems:nil];
}

- (id)initWithMainItem:(NJBQueueItem *)newMainItem withSubItems:(NSArray *)newSubItems
{
	if (self = [super init])
	{
		[self setMainItem:newMainItem];
		[self setSubItems:newSubItems];
		status = ITEM_STATUS_QUEUED;
	}
	return self;
}

- (void)dealloc
{
	[mainItem release];
	[subItems release];
	[description release];
	[resultString release];
	[super dealloc];
}

// accessor methods

- (void)setSubItems:(NSArray *)newSubItems
{
	[newSubItems retain];
	[subItems release];
	subItems = newSubItems;
}

- (void)setMainItem:(NJBQueueItem *)newMainItem
{
	[newMainItem retain];
	[mainItem release];
	mainItem = newMainItem;
}

- (NJBQueueItem *)mainItem
{
	return mainItem;
}

- (itemStatusTypes)status
{
	return status;
}

- (void)setStatus:(itemStatusTypes)newStatus
{
	status = newStatus;
}

- (NSString *)description
{
	return description;
}

- (void)setDescription:(NSString *)newDescription
{
	if (newDescription == nil)
		newDescription = @"";
	// get immutable copy
	newDescription = [NSString stringWithString:newDescription];
	[newDescription retain];
	[description release];
	description = newDescription;
}

- (void)setResultString:(NSString *)newResultString
{
	if (newResultString == nil)
		newResultString = @"";
	// get immutable copy
	newResultString = [NSString stringWithString:newResultString];
	[newResultString retain];
	[resultString release];
	resultString = newResultString;
}

- (NSString *)resultString
{
	return resultString;
}

/**********************/

- (void)cancel
{
	[mainItem cancel];
	NSEnumerator *enumerator = [subItems objectEnumerator];
	NJBQueueItem *currentItem;
	while (currentItem = [enumerator nextObject])
	{
		[currentItem cancel];
	}
	[self setStatus:ITEM_STATUS_CANCELLED];
}

@end
