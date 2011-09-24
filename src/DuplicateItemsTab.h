//
//  DuplicateItemsTab.h
//  XNJB
//
//  Created by Richard Low on 20/02/2005.
//

#import <Cocoa/Cocoa.h>
#import "MyTab.h"
#import "NJBQueueItem.h"
#import "Track.h"
#import "MyBool.h"

@interface DuplicateItemsTab : MyTab {
	IBOutlet NSTableView *njbTable;
	IBOutlet NSButton *findDuplicatesButton;
	IBOutlet NSButton *deleteButton;
	IBOutlet NSTextField *findDuplicatesStatusField;
	NSMutableArray *itemArray;
	NSMutableArray *duplicatesArray;
	BOOL trackListUpToDate;
	// this is alloced set when we call the duplicate track finding routine
	// in another thread. If we set the value to false then the thread should quit
	MyBool *continueFindingDuplicateTracks;
}
- (void)loadTracks;
- (NJBTransactionResult *)downloadTrackList;
- (void)findDuplicates;
- (void)findDuplicatesWorker:(NSArray *)items;
- (IBAction)findDuplicatesButton:(id)sender;
- (void)enableFindDuplicatesButton;
- (void)deleteFromNJB:(Track *)track;
- (void)tableViewSelectionDidChange:(NSNotification *)aNotification;
- (void)replaceTrack:(Track *)replace withTrack:(Track *)new;
- (void)drawerWriteTag:(Track *)modifiedTrack;
- (void)deleteFromNJB;
- (IBAction)deleteButton:(id)sender;
- (void)menuDelete;
- (BOOL)menuShouldDelete;
- (void)reloadData;

@end
