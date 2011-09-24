//
//  QueueTabItem.h
//  XNJB
//
//  Created by Richard Low on Wed Jul 28 2004.
//

#import <Foundation/Foundation.h>
#import "NJBQueueItem.h"

typedef enum _itemStatusTypes {ITEM_STATUS_QUEUED = 0, ITEM_STATUS_RUNNING = 1, ITEM_STATUS_FINISHED_WITH_SUCCESS = 2,
															 ITEM_STATUS_FINISHED_WITH_FAILURE = 3, ITEM_STATUS_CANCELLED = 4} itemStatusTypes;

@interface QueueTabItem : NSObject {
	NJBQueueItem *mainItem;
	NSArray *subItems;
	NSString *description;
	NSString *resultString;
	itemStatusTypes status;
}
- (id)initWithMainItem:(NJBQueueItem *)newMainItem withSubItems:(NSArray *)newSubItems;
- (void)setSubItems:(NSArray *)newSubItems;
- (void)setMainItem:(NJBQueueItem *)newMainItem;
- (NJBQueueItem *)mainItem;
- (void)cancel;
- (itemStatusTypes)status;
- (void)setStatus:(itemStatusTypes)status;
- (NSString *)description;
- (void)setDescription:(NSString *)descriptions;
- (void)setResultString:(NSString *)newResultString;
- (NSString *)resultString;

@end
