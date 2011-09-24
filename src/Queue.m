//
//  Queue.m
//  QueueTest
//
//  Created by Richard Low on Thu Jul 22 2004.
//

#import "Queue.h"

/* Implements a FIFO queue
 * Inefficient as uses an array
 * but generally our queues are small.
 * Could implement with a linked list
 * If used in multithreaded code must be
 * combined with a lock
 */
@implementation Queue

// init/dealloc methods

- (id)init
{
	if (self = [super init])
	{
		queue = [[NSMutableArray alloc] init];
	}
	return self;
}

- (void)dealloc
{
	[queue dealloc];
	[super dealloc];
}

/**********************/

/* returns the autorelease object at the head of the queue
 */
- (id)nextObject
{
	if ([self isEmpty])
		return nil;
	id head = [queue objectAtIndex:0];
	[head retain];
	[queue removeObjectAtIndex:0];
	return [head autorelease];
}

- (void)addObject:(id)newObject
{
	[queue addObject:newObject];
}

- (BOOL)isEmpty
{
	if ([queue count] == 0)
		return YES;
	else
		return NO;
}

- (unsigned)length
{
	return [queue count];
}

- (void)removeAtIndex:(unsigned)index
{
	[queue removeObjectAtIndex:index];
}

@end
