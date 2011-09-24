//
//  MyTrackTab.m
//  XNJB
//
//  Created by Richard Low on 04/09/2004.
//

/* This class is for a tab which has an
 * NSTableView used to show items on the
 * NJB e.g. tracks or datafiles. It allows
 * sorting and searching of the list.
 */

#import "defs.h"
#import "MyTableTab.h"

@implementation MyTableTab

// init/dealloc methods

- (void)dealloc
{
	[tempArray release];
	[super dealloc];
}

/**********************/

/* to be implemented by subclass:
 * search the njbTable for strings
 * containing the contents of njbSearchField
 * called when the search box has been modified
 */
- (void)searchNJBTable{}

- (void)sortArrays
{
	[arrayController rearrangeObjects];
}

/* called when we connect to an NJB
 * clear the outdated arrays
 */
- (void)NJBConnected:(NSNotification *)note
{
	// clear the list if there was one so we can't change stuff while getting new file list
	[arrayController removeAll];
	[njbTable reloadData];
	
	// this must be done last as we might set fullItemArray in onConnectAndActive
	[super NJBConnected:note];
}

/* delegate method called when
 * the search box is modified
 */
- (IBAction)searchNJBTableAction:(id)sender
{
	// njbSearchField has been changed
	[arrayController search:sender];
	
	[self showItemCounts];
}

- (void)showNewArray
{
	[arrayController setContent:tempArray];
	[tempArray release];
	tempArray = nil;
	[self searchNJBTable];
	[self sortArrays];
	
	[self showItemCounts];
}

- (void)tableViewSelectionDidChange:(NSNotification *)aNotification
{
	[self showItemCounts];
}

- (void)showItemCounts
{
	NSString *selectedString = [NSString stringWithFormat:NSLocalizedString(@"Selected %d out of %d", nil), [njbTable numberOfSelectedRows], [[arrayController arrangedObjects] count]];
	[itemCountField setStringValue:selectedString];
}


@end
