//
//  QueueTab.h
//  XNJB
//

#import <Cocoa/Cocoa.h>
#import "MyTab.h"
#import "NJBQueueItem.h"
#import "StatusDisplayer.h"
#import "QueueTabItem.h"

@interface QueueTab : MyTab
{
	NSMutableArray *queueItems;
	IBOutlet NSTableView *queueTable;
	IBOutlet NSProgressIndicator *mainProgressIndicator;
	IBOutlet StatusDisplayer *statusDisplayer;
	int currentTaskIndex;
	
	NSImage *imageQueued;
	NSImage *imageRunning;
	NSImage *imageSuccess;
	NSImage *imageFailure;
	NSImage *imageCancelled;
}
- (void)addItem:(NJBQueueItem *)mainItem withSubItems:(NSArray *)subItems withDescription:(NSString *)description;
- (int)numberOfRowsInTableView:(NSTableView *)aTableView;
- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn	row:(int)rowIndex;
- (NSString *)statusStringForStatus:(itemStatusTypes)status;
- (void)taskComplete:(NSNotification *)note;
- (void)taskStarting:(NSNotification *)note;
- (BOOL)menuShouldCancelQueuedItem;
- (void)menuCancelQueuedItem;
- (NSArray *)selectedItemsToCancel;
@end
