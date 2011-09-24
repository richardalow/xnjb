//
//  NJBQueueItem.h
//  QueueTest
//
//  Created by Richard Low on Thu Jul 22 2004.
//

#import <Foundation/Foundation.h>
#import "StatusDisplayer.h"

@interface NJBQueueItem : NSObject {
	id target;
	SEL selector;
	id object1;
	id object2;
	BOOL runInMainThread;
	statusTypes status;
	BOOL displayStatus;
	BOOL cancelled;
	NSMutableArray *itemsToCancelIfFail;
}
- (void)setTarget:(id)newTarget;
- (void)setSelector:(SEL)newSelector;
- (void)setObject1:(id)newObject1;
- (void)setObject2:(id)newObject2;
- (void)setStatus:(statusTypes)newStatus;
- (statusTypes)status;
- (void)setRunInMainThread:(BOOL)newRunInMainThread;
- (BOOL)runInMainThread;
- (void)setDisplayStatus:(BOOL)newDisplayStatus;
- (BOOL)displayStatus;
- (id)process;
- (id)initWithTarget:(id)newTarget withSelector:(SEL)newSelector;
- (id)initWithTarget:(id)newTarget withSelector:(SEL)newSelector withObject:(id)newObject;
- (id)initWithTarget:(id)newTarget withSelector:(SEL)newSelector withObject:(id)newObject withRunInMainThread:(BOOL)newRunInMainThread;
- (void)cancel;
- (BOOL)cancelled;
- (void)cancelItemIfFail:(NJBQueueItem *)item;

@end
