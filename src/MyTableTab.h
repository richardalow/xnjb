//
//  MyTrackTab.h
//  XNJB
//
//  Created by Richard Low on 04/09/2004.
//

#import <Cocoa/Cocoa.h>
#import "MyTab.h"
#import "MyNSArrayController.h"

@interface MyTableTab : MyTab {
  IBOutlet NSTableView *njbTable;
	IBOutlet NSTextField *njbSearchField;
	IBOutlet NSTextField *itemCountField;
	IBOutlet MyNSArrayController *arrayController;
	NSMutableArray *tempArray;
}
- (void)searchNJBTable;
- (IBAction)searchNJBTableAction:(id)sender;
- (void)sortArrays;
- (void)showNewArray;
- (void)showItemCounts;

@end
