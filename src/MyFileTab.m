//
//  MyFileTab.m
//  XNJB
//

/* This is for a tab that has both an NSTableView for jukebox items and a FileSystemBrowser 
 * for local items. Implements methods for copying between the two and deleting.
 */

#import "MyFileTab.h"
#import "defs.h"
#import "MyNSWindow.h"

@implementation MyFileTab

// init/dealloc methods

- (id)init
{
	if (self = [super init])
	{
		NSNotificationCenter *nc;
		nc = [NSNotificationCenter defaultCenter];
		[nc addObserver:self selector:@selector(applicationTerminating:) name:NOTIFICATION_APPLICATION_TERMINATING object:nil];
	}
	return self;
}

- (void)dealloc
{
	[browser release];
	[super dealloc];
}

/**********************/

- (void)awakeFromNib
{
	[super awakeFromNib];
	browser = [[FileSystemBrowser alloc] initWithBrowser:fileBrowser atLocation:[self browserPathFromPreferences]];
	[browser setTarget:self];
	[browser setFileAction:@selector(fileSystemBrowserFileClick:)];
	[browser setDirectoryAction:@selector(fileSystemBrowserDirectoryClick:)];
	
	// allow drag & drop
	[njbTable registerDragAndDrop];
	[njbTable disallowCopies];
}

/* makes a string into a legal filename
 * this is the filename excluding directory
 */
- (NSString *)replaceBadFilenameChars:(NSString *)filename
{
	if (filename == nil || [filename length] == 0)
		return @"";
	NSMutableString *file = [[NSMutableString alloc] initWithString:filename];
	[file replaceOccurrencesOfString:@"/" withString:@"_" options:NSCaseInsensitiveSearch range:NSMakeRange(0, [filename length])];
	[file replaceOccurrencesOfString:@":" withString:@"_" options:NSCaseInsensitiveSearch range:NSMakeRange(0, [filename length])];
	NSString *f = [NSString stringWithString:file];
	[file release];
	
	return f;
}

/* To be implemented by super class:
 * called by MainController when the user clicks on 
 * the write button in the tag drawer
 */
- (void)drawerWriteTag:(Track *)replaceTrackAtIndex{}

/* called when the user clicks on the delete menu
 */
- (void)menuDelete
{
	switch ([self activeObject])
	{
		case OBJECT_FILE_BROWSER:
			[self deleteFromFileSystem];
			break;
		case OBJECT_NJB_TABLE:
			[self deleteFromNJB];
			break;
		default:
			break;
	}
}

/* can we delete something?
 */
- (BOOL)canDelete
{
	switch ([self activeObject])
	{
		case OBJECT_NJB_TABLE:
			if (![theNJB isConnected])
				return NO;
			if ([njbTable selectedRow] != -1)
				return YES;
			else
				return NO;
			break;
		case OBJECT_FILE_BROWSER:
			if ([browser directorySelected])
				return NO;
			else
				return YES;
			break;
		default:
			return NO;
	}		
}

- (BOOL)menuShouldDelete
{
	return [self canDelete];
}

/* called by onFirstResponderChange: to tell us which object is
 * currently active
 */
- (void)setActiveObject:(activeObjects)newActiveObject
{
	activeObject = newActiveObject;
}

- (activeObjects)activeObject
{
	return activeObject;
}

- (void)onFirstResponderChange:(NSResponder *)aResponder
{
	// don't do anything if it's the window - we will have been called with another object
	if ([aResponder class] == [MyNSWindow class])
		return;
	if (aResponder == njbTable)
		[self setActiveObject:OBJECT_NJB_TABLE];
	else if ([aResponder class] == [NSMatrix class])
		[self setActiveObject:OBJECT_FILE_BROWSER];
	else
		[self setActiveObject:OBJECT_OTHER];
	[deleteFromActiveObjectButton setEnabled:[self canDelete]];
	
	if (aResponder == njbTable)
	{
		[self showDrawerInfo];
	}
	// used to be [aResponder class] == [NSSearchField class]
	else if (aResponder == njbSearchField)
	{
		[drawerController disableAll];
	}
}

/* can we copy a file to the NJB?
 */
- (BOOL)canCopyToNJB
{
	if (![theNJB isConnected])
		return NO;
	else
		return ([browser path] != nil && [[browser path] length] > 0);
}

/* can we copy a file from the NJB?
 */
- (BOOL)canCopyFromNJB
{
	if (![theNJB isConnected])
		return NO;
	else
		return ([njbTable selectedRow] != -1);
}

/* called when we disconnect
 */
- (void)NJBDisconnected:(NSNotification *)note
{
	[super NJBDisconnected:note];
	[self disableTransfers];
}

/* called when we connect
 */
- (void)NJBConnected:(NSNotification *)note
{
	[super NJBConnected:note];
	[self enableTransfers];
}

/* things to do when we terminate (implemented by superclass)
 */
- (void)applicationTerminating:(NSNotification *)note{}

/* enable the buttons to allow transfers if connected
 */
- (void)enableTransfers
{
	if (![theNJB isConnected])
	{
		[self disableTransfers];
		return;
	}
	[deleteFromActiveObjectButton setEnabled:[self canDelete]];
	[copyToNJBButton setEnabled:[self canCopyToNJB]];
	[copyFromNJBButton setEnabled:[self canCopyFromNJB]];
	[njbTable allowCopies];
}

/* disable transfer buttons
 */
- (void)disableTransfers
{
	[deleteFromActiveObjectButton setEnabled:[self canDelete]];
	[copyToNJBButton setEnabled:NO];
	[copyFromNJBButton setEnabled:NO];
	[njbTable disallowCopies];
}

/* get a list of all the files to copy to the NJB
 */
- (IBAction)copyToNJB:(id)sender
{
	NSArray *filesToCopy = [browser selectedFilesWithRecursion:YES];
	NSString *currentFile;
	NSEnumerator *enumerator = [filesToCopy objectEnumerator];
	while (currentFile = [enumerator nextObject])
	{
		[self copyFileToNJB:currentFile];
	}
}

/* actually copy the file, implemented by superclass
 */
- (void)copyFileToNJB:(NSString *)filename{}

/* get a list of all the files to copy from the NJB
 */
- (IBAction)copyFromNJB:(id)sender
{
	NSArray *selectedObjects;
	selectedObjects = [arrayController selectedObjects];
	NSEnumerator *trackEnumerator = [selectedObjects objectEnumerator];
	MyItem *item;
	while (item = [trackEnumerator nextObject])
	{
		[self copyFileFromNJB:item];
	}
}

/* actually copy the file, implemented by superclass
 */ 
- (void)copyFileFromNJB:(MyItem *)item{}

/* delete a file locally
 * will delete all files in a directory but not directories or recursively
 */
- (void)deleteFromFileSystem
{
	if ([browser directorySelected])
	{
		// should not be here
		NSLog(@"Error: deleting local files when directory selected!");
		return;
	}
	int result = NSRunAlertPanel(NSLocalizedString(@"File Deletion", nil),
															 NSLocalizedString(@"Are you sure you want to delete the selected local files?", nil),
															 NSLocalizedString(@"No", nil),
															 NSLocalizedString(@"Yes", nil), nil);
	if (result == NSAlertDefaultReturn)
		return;
	
	NSFileManager *fileManager = [[NSFileManager alloc] init];
	NSArray *selectedFiles = [browser selectedFilesWithRecursion:NO];
	NSEnumerator *enumerator = [selectedFiles objectEnumerator];
	NSString *currentFile;
	while (currentFile = [enumerator nextObject])
	{
		if (![fileManager removeFileAtPath:currentFile handler:nil])
		{
			NSRunAlertPanel(NSLocalizedString(@"File Deletion", nil),
											[NSString stringWithFormat:NSLocalizedString(@"There was an error deleting file %@.", nil), currentFile], NSLocalizedString(@"OK", nil), nil, nil);
			return;
		}
	}
	[fileManager release];
	[browser reloadData];
	[self enableTransfers];
}

/* delete all the selected items off the NJB, after warning
 */
- (void)deleteFromNJB
{
	int result = NSRunAlertPanel(NSLocalizedString(@"File Deletion", nil),
															 NSLocalizedString(@"Are you sure you want to delete the selected items off the NJB?", nil),
															 NSLocalizedString(@"No", nil),
															 NSLocalizedString(@"Yes", nil), nil);
	if (result == NSAlertDefaultReturn)
		return;
	
	/* get array of items to be deleted
	 * we can't pass indexes as they aren't valid
	 * after items have been deleted
	 */
	NSArray *selectedObjects = [arrayController selectedObjects];
	NSEnumerator *enumerator = [selectedObjects objectEnumerator];
	MyItem *currentItem;
	while (currentItem = [enumerator nextObject])
	{
		[self deleteFromNJB:currentItem];
	}
}

/* delete an item off the NJB, to be implemented by superclass
 */
- (void)deleteFromNJB:(MyItem *)item{}

/* clicked on a track/file so we can transfer
 */
- (void)tableViewSelectionDidChange:(NSNotification *)aNotification
{
	// we might be called from code changing the selection
	if (activeObject != OBJECT_NJB_TABLE && activeObject != OBJECT_FILE_BROWSER)
		return;

//	if ([njbTable numberOfSelectedRows] > 0)
	[self enableTransfers];
	
	[self showDrawerInfo];
	
	[super tableViewSelectionDidChange:aNotification];
}

/* clicked on a local track/file so we can transfer
 */
- (void)fileSystemBrowserFileClick:(NSString *)path
{
	[self enableTransfers];
}

/* clicked on a local directory so we can transfer
 */
- (void)fileSystemBrowserDirectoryClick:(NSString *)path
{
	[self enableTransfers];
}

- (void)deactivate
{
	[super deactivate];
	[drawerController disableAll];
}

- (NSString *)browserDirectory
{
	return [browser directory];
}

/* user clicked on delete button
 */
- (IBAction)deleteFromActiveObject:(id)sender
{
	[self menuDelete];
}

/* adds the item to the arrays and sorts
 * if necessary
 */
- (void)addToArray:(MyItem *)item
{
	[arrayController addObject:item];
	[arrayController search:njbSearchField];
	[self sortArrays];
	[self showItemCounts];
}

- (void)removeFromArray:(MyItem *)item
{
	[arrayController removeObject:item];
	[self showItemCounts];
}

- (void)activate
{
	[super activate];
	if (activeObject == OBJECT_NJB_TABLE)
	{
		[self showDrawerInfo];
	}
}

/* show the info for the selected file/track in the table in the drawer
 * to be implemented by super class
 */
- (void)showDrawerInfo{}

- (void)tableView:(NSTableView *)tableView didClickTableColumn:(NSTableColumn *)tableColumn
{	
	// disable and clear the drawer
	[drawerController clearAll];
	[drawerController disableAll];
}

/*
 * return the browser path stored from preferences
 * to be implemented by super class
 */
- (NSString *)browserPathFromPreferences{ return @"/"; }

@end
