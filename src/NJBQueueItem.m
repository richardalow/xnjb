//
//  NJBQueueItem.m
//  QueueTest
//
//  Created by Richard Low on Thu Jul 22 2004.
//

/* The items added to the NJB queue
 * process them by sending the process message
 * Note that setting object2 to nil will cause us
 * to only send object1 which may result in object1
 * being passed as both arguments. So don't set object2
 * to nil unless your function only has one argument.
 */

#import "NJBQueueItem.h"
#import "NJBTransactionResult.h"

@implementation NJBQueueItem

// init/dealloc methods

- (id)init
{
	return [self initWithTarget:nil withSelector:nil withObject:nil withRunInMainThread:NO];
}

- (id)initWithTarget:(id)newTarget withSelector:(SEL)newSelector
{
	return [self initWithTarget:newTarget withSelector:newSelector withObject:nil withRunInMainThread:NO];
}

- (id)initWithTarget:(id)newTarget withSelector:(SEL)newSelector withObject:(id)newObject
{
	return [self initWithTarget:newTarget withSelector:newSelector withObject:newObject withRunInMainThread:NO];
}

- (id)initWithTarget:(id)newTarget withSelector:(SEL)newSelector withObject:(id)newObject withRunInMainThread:(BOOL)newRunInMainThread
{
	if (self = [super init])
	{
    object1 = nil;
    object2 = nil;
    target = nil;
    itemsToCancelIfFail = nil;
    
		[self setTarget:newTarget];
		[self setSelector:newSelector];
		[self setObject1:newObject];
		[self setRunInMainThread:newRunInMainThread];
		[self setDisplayStatus:YES];
		[self setStatus:STATUS_UNKNOWN];
		cancelled = NO;
		
		itemsToCancelIfFail = [[NSMutableArray alloc] init];
	}
	return self;
}

- (void)dealloc
{
	// most likely, this has been processed or cancelled so these have all been set to nil. Just in case...
	[target release];
	[object1 release];
	[object2 release];
	[itemsToCancelIfFail release];
	[super dealloc];
}

// accessor methods

- (void)setTarget:(id)newTarget
{
	[newTarget retain];
	[target release];
	target = newTarget;
}

- (void)setSelector:(SEL)newSelector
{
	selector = newSelector;
}

- (void)setObject1:(id)newObject1
{
	[newObject1 retain];
	[object1 release];
	object1 = newObject1;
}

- (void)setObject2:(id)newObject2
{
	[newObject2 retain];
	[object2 release];
	object2 = newObject2;
}

- (void)setStatus:(statusTypes)newStatus
{
	status = newStatus;
}

- (statusTypes)status
{
	return status;
}

- (void)setRunInMainThread:(BOOL)newRunInMainThread
{
	runInMainThread = newRunInMainThread;
}

- (BOOL)runInMainThread
{
	return runInMainThread;
}

- (void)setDisplayStatus:(BOOL)newDisplayStatus
{
	displayStatus = newDisplayStatus;
}

- (BOOL)displayStatus
{
	return displayStatus;
}

/**********************/

/* processes the message in this thread (ignores runInMainThread)
 * releases target, object1 and object2
 */
- (id)process
{
	if ([self cancelled])
		return nil;

	id ret;
	//if (object1 == nil)
	//	ret = [target performSelector:selector];
	//else if (object2 == nil)
	//	ret = [target performSelector:selector withObject:object1];
	//else
		ret = [target performSelector:selector withObject:object1 withObject:object2];
	
	[target release];
	target = nil;
	[object1 release];
	object1 = nil;
	[object2 release];
	object2 = nil;
	
	// this will crash if ret is not a class object, is there any way I can test this?
	if ([itemsToCancelIfFail count] > 0 && [ret isKindOfClass:[NJBTransactionResult class]] && ![ret success])
	{
		NSEnumerator *enumerator = [itemsToCancelIfFail objectEnumerator];
		NJBQueueItem *item;
		while (item = [enumerator nextObject])
			[item cancel];
	}
	// release all items now
	[itemsToCancelIfFail release];
	itemsToCancelIfFail = nil;
	
	return ret;
}

- (void)cancel
{
	cancelled = YES;
	[target release];
	target = nil;
	[object1 release];
	object1 = nil;
	[object2 release];
	object2 = nil;
	[itemsToCancelIfFail release];
	itemsToCancelIfFail = nil;
}

- (BOOL)cancelled
{
	return cancelled;
}

- (void)cancelItemIfFail:(NJBQueueItem *)item
{
	[itemsToCancelIfFail addObject:item];
}

/* retain the objects and target we have. This solves a complicated problem:
 * If I create this object in one thread, by adding autoreleased objects, they get retain counts of 2
 * then I add this object to the queue of the other thread, retaining me but not my objects (without this function)
 * then I am released in the original thread, reducing the retain count of my objects to 1 which are then autoreleased
 * which kills them.  So this is required.  Safe because I will be released and retained the same number of times (if I am
 * to be freed).  Same for the objects.																																																								 
 * UPDATE: I can't remember what the problems were, but this causes many memory leaks.  Added in releases on release of me to cancel out.
 */
/*- (id)retain
{
	[object1 retain];
	[object2 retain];
	[target retain];
	return [super retain];
}*/
/*
- (void)release
{
  [object1 release];
  [object2 release];
  [target release];
  return [super release];
}*/

@end
