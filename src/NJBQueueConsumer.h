//
//  QueueConsumer.h
//  QueueTest
//
//  Created by Richard Low on Thu Jul 22 2004.
//

#import <Foundation/Foundation.h>
#import "Queue.h"
#import "StatusDisplayer.h"

#define NO_QUEUE_ITEMS 0
#define SOME_QUEUE_ITEMS 1
#define PROCESSING_QUEUE 2

@interface NJBQueueConsumer : NSObject {
	Queue *theQueue;
	NSConditionLock *queueLock;
	id mainThreadProxy;
	NSConnection *serverConnection;
	StatusDisplayer *status;
	Queue *mainThreadQueue;
	NSLock *mainThreadQueueLock;
}
- (id)initWithQueue:(Queue *)newQueue withLock:(NSConditionLock *)newLock;
- (void)setQueue:(Queue *)newQueue;
- (Queue *)queue;
- (void)setLock:(NSConditionLock *)newLock;
- (NSConditionLock *)lock;
- (void)consume;
- (void)consumeQueue:(Queue *)queue;
- (void)connectToMainThreadWithPorts:(NSArray *)portArray;
- (void)setStatusDisplayer:(StatusDisplayer *)newStatus;
- (void)setMainThreadQueue:(Queue *)newMainThreadQueue;
- (void)setMainThreadQueueLock:(NSLock *)newMainThreadQueueLock;
@end
