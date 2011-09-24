//
//  Queue.h
//  QueueTest
//
//  Created by Richard Low on Thu Jul 22 2004.
//

#import <Foundation/Foundation.h>

@interface Queue : NSObject {
  NSMutableArray *queue;
}
- (id)nextObject;
- (BOOL)isEmpty;
- (void)addObject:(id)newObject;
- (void)removeAtIndex:(unsigned)index;
- (unsigned)length;

@end
