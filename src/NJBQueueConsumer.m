//
//  QueueConsumer.m
//  QueueTest
//
//  Created by Richard Low on Thu Jul 22 2004.
//

/* This class is a queue consumer that
 * runs on queues of type Queue.
 * Runs in its own thread.
 * Uses an NSCoditionLock in an inifinte loop.
 * Talks to main thread via a proxy object.
*/

#import "NJBQueueConsumer.h"
#import "NJBQueueItem.h"
#import "NJBTransactionResult.h"

@implementation NJBQueueConsumer

// init/dealloc methods

- (id)initWithQueue:(Queue *)newQueue withLock:(NSConditionLock *)newLock
{
	if (self = [super init])
	{
		[self setQueue:newQueue];
		[self setLock:newLock];
	}
	return self;
}

- (void)dealloc
{
	[theQueue release];
	[queueLock release];
	[mainThreadProxy release];
	[serverConnection release];
	[status release];
	[mainThreadQueue release];
	[mainThreadQueueLock release];
	[super dealloc];
}

// accessor methods

- (void)setQueue:(Queue *)newQueue
{
	[newQueue retain];
	[theQueue release];
	theQueue = newQueue;
}

- (Queue *)queue
{
	return theQueue;
}

- (void)setLock:(NSConditionLock *)newLock
{
	[newLock retain];
	[queueLock release];
	queueLock = newLock;
}

- (NSConditionLock *)lock
{
	return queueLock;
}

- (void)setStatusDisplayer:(StatusDisplayer *)newStatus
{
	[newStatus retain];
	[status release];
	status = newStatus;
}

- (void)setMainThreadQueue:(Queue *)newMainThreadQueue
{
	[newMainThreadQueue retain];
	[mainThreadQueue release];
	mainThreadQueue = newMainThreadQueue;
}

- (void)setMainThreadQueueLock:(NSLock *)newMainThreadQueueLock
{
	[newMainThreadQueueLock retain];
	[mainThreadQueueLock release];
	mainThreadQueueLock = newMainThreadQueueLock;
}

/**********************/

/* called from main thread to start consuming
 * the queue
 */
- (void)consume
{
	[NSThread detachNewThreadSelector:@selector(consumeQueue:) toTarget:self withObject:theQueue];
}

/* The run loop of the thread.
 * We spend all our time in this method
 */
- (void)consumeQueue:(Queue *)queue
{
	while (true)
	{
		// create a new pool on each cycle
		// so we don't end up with loads of autoreleased
		// object when we get killed
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
		
		[queueLock lockWhenCondition:SOME_QUEUE_ITEMS];
    //NSLog(@"queue locked by NJBQueueConsumer consumeQueue");
		// Remove data from the queue.
		NJBQueueItem *njbQueueItem = [queue nextObject];
		[queueLock unlockWithCondition:PROCESSING_QUEUE];
    //NSLog(@"queue unlocked by NJBQueueConsumer consumeQueue");
		
		if (![njbQueueItem cancelled])
		{
			if ([njbQueueItem displayStatus])
			{
				[status startTask];
				[status setStatus:[njbQueueItem status]];
			}
			
			if ([njbQueueItem runInMainThread])
			{
				// add to main thread queue
				[mainThreadQueueLock lock];
				[mainThreadQueue addObject:njbQueueItem];
				[mainThreadQueueLock unlock];
				// tell the main thread there are queue items
//				NSLog(@"NJBQueueConsumer telling mainThreadProxy to consume queue");
				[mainThreadProxy consumeQueue];
//				NSLog(@"mainThreadProxy has returned");
				if ([njbQueueItem displayStatus])
					NSLog(@"displayStatus == YES for runInMainThread queue item!");
			}
			else
			{
//				NSLog(@"processing njbQueueItem");
				id result = [njbQueueItem process];
//				NSLog(@"returned from processing njbQueueItem");
				if ([njbQueueItem displayStatus])
				{
					if ([result class] == [NJBTransactionResult class])
						[status taskComplete:result];
					else
						[status taskComplete:[[[NJBTransactionResult alloc] initWithSuccess:YES] autorelease]];
					}
			}
		}
		
    [queueLock lock];
      BOOL empty = [queue isEmpty];
    [queueLock unlockWithCondition:([queue isEmpty] ? NO_QUEUE_ITEMS : SOME_QUEUE_ITEMS)];
    if (empty)
      [status setIdle];
		
//		NSLog(@"going round queue loop again...");
    [pool release];
	}
	// never gets here...
	NSLog(@"Got to end of run loop!");
}

/* called when initializing to set up the
 * proxy object
 */
- (void)connectToMainThreadWithPorts:(NSArray *)portArray
{
	serverConnection = [NSConnection connectionWithReceivePort:[portArray objectAtIndex:0]
																										sendPort:[portArray objectAtIndex:1]];
	[serverConnection retain];
	mainThreadProxy = [serverConnection rootProxy];
	[mainThreadProxy retain];
	
	[status setMainThreadProxy:mainThreadProxy];
}

@end
