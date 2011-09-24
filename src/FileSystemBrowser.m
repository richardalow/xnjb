//
//  FileSystemBrowser.m
//  XNJB
//
//  Created by Richard Low on Tue Jul 20 2004.
//

/* this class makes an NSBrowser into a file system
 * browser with icons, etc.
 */

#import "FileSystemBrowser.h"
#import "FileSystemBrowserNode.h"
#import "FileSystemBrowserCell.h"

// declare the private methods
@interface FileSystemBrowser (PrivateAPI)
- (NSString*)pathToColumn:(int)column;
@end

@implementation FileSystemBrowser

// init/dealloc methods

- (id)init
{
	return [self initWithBrowser:nil atLocation:@"/"];
}

- (id)initWithBrowser:(NSBrowser *)newBrowser
{
	return [self initWithBrowser:newBrowser atLocation:@"/"];
}

- (id)initWithBrowser:(NSBrowser *)newBrowser atLocation:(NSString *)newLocation
{
	if (self = [super init])
	{
		[self setBrowser:newBrowser];
		[self setBaseLocation:newLocation];
	}
	return self;
}

- (void)dealloc
{
	[browser release];
	[baseLocation release];
	[super dealloc];
}

// accessor methods

/* the target for file/directory click
 * messages
 */
- (void)setTarget:(id)newTarget
{
	clickTarget = newTarget;
}

/* set selector for file click
 */
- (void)setFileAction:(SEL)newFileAction
{
	clickFileAction = newFileAction;
}

/* set selector for directory click
*/
- (void)setDirectoryAction:(SEL)newDirectoryAction
{
	clickDirectoryAction = newDirectoryAction;
}

/* set the NSBrowser we are attached to
 */
- (void)setBrowser:(NSBrowser *)newBrowser
{
	[newBrowser retain];
	[browser release];
	browser = newBrowser;
	[browser setDelegate:self];
	[browser setTarget:self];
	[browser setAction:@selector(browserClick:)];
	[browser setCellClass:[FileSystemBrowserCell class]];
}

- (NSBrowser *)browser
{
	return browser;
}

/* move to this dir
 */
- (void)setBaseLocation:(NSString *)newBaseLocation
{
	//NSLog(@"NSBrowser setBaseLocation");
	
	if (newBaseLocation == nil)
		newBaseLocation = @"";
	// get immutable copy and expand the ~ in path if there is one
	newBaseLocation = [newBaseLocation stringByExpandingTildeInPath];
	[newBaseLocation retain];
	[baseLocation release];
	baseLocation = newBaseLocation;

	// here we select the rows manually
	// this is because doing a NSBrowser setPath causes
	// *all* lower nodes to be loaded, and this can take a long
	// time for large directories.
	
	// start at the bottom
	[browser setPath:@"/"];
	
	// done if we don't have anything to select
	if ([newBaseLocation length] == 0)
		return;
	
	NSArray *pathComponents = [newBaseLocation pathComponents];
	NSEnumerator* enumerator = [pathComponents objectEnumerator];
	NSString *component = [enumerator nextObject];
	
	// give up if we don't have an absolute path
	if (![component isEqualToString:@"/"])
	{
		return;
	}
	
	// go through each level one by one, selecting where we should be
	// although this goes through the whole directory structure, just as it does when
	// we setPath, this avoids overhead of creating the cells and icons so is much
	// faster.
	FileSystemBrowserNode *parentNode = [FileSystemBrowserNode nodeWithParent:nil atRelativePath:@"/"];
	int column = 0;
	while (component = [enumerator nextObject])
	{
		int index = [parentNode rowIndexForNode:component];
		if (index == -1)
			break;
		
		[browser selectRow:index inColumn:column];
		
		parentNode = [FileSystemBrowserNode nodeWithParent:parentNode atRelativePath:component];
	
		column++;
	}
	//[browser setPath:baseLocation];
	// no need to reload data as setPath: will
}

- (NSString *)baseLocation
{
	return baseLocation;
}

- (void)reloadData
{
	[browser reloadColumn:[browser selectedColumn]];
}

- (NSString *)path
{
	return [browser path];
}

- (NSString *)directory
{
	if ([self directorySelected])
		return [self path];
	else
	{
		int selectedColumn = [browser selectedColumn];
		if (selectedColumn == 0)
			return @"/";
		else
			return [self pathToColumn:selectedColumn];
	}
}

/**********************/

// action
- (IBAction)browserClick:(id)aBrowser
{
	if (clickTarget == nil)
		return;
	
	NSString *path = [aBrowser path];
	FileSystemBrowserNode *node = [FileSystemBrowserNode nodeWithParent:nil atRelativePath:path];
	
	if ([node isDirectory])
	{
		if ([clickTarget respondsToSelector:clickDirectoryAction])
			[clickTarget performSelector:clickDirectoryAction withObject:[aBrowser path]];
	}
	else
	{
		if ([clickTarget respondsToSelector:clickFileAction])
			[clickTarget performSelector:clickFileAction withObject:[aBrowser path]];
	}
}

// delegate methods
- (int)browser:(NSBrowser *)sender numberOfRowsInColumn:(int)column {	
	NSString *path = nil;
	FileSystemBrowserNode *node = nil;
	
	// Get the absolute path represented by the browser selection, and create a FileSystemBrowserNode for the path.
	// Since column represents the column being (lazily) loaded path is the path for the last selected cell.
	path = [self pathToColumn:column];
	node = [FileSystemBrowserNode nodeWithParent:nil atRelativePath:path];
	
	return [[node visibleSubNodes] count];
}

- (void)browser:(NSBrowser *)sender willDisplayCell:(id)cell atRow:(int)row column:(int)column {
	NSString   *containingDirPath = nil;
	FileSystemBrowserNode *containingDirNode = nil;
	FileSystemBrowserNode *displayedCellNode = nil;
	NSArray    *directoryContents = nil;
	
	// Get the absolute path represented by the browser selection, and create a FileSystemBrowserNode for the path.
	// Since (row,column) represents the cell being displayed, containingDirPath is the path to it's containing directory.
	containingDirPath = [self pathToColumn:column];
	containingDirNode = [FileSystemBrowserNode nodeWithParent:nil atRelativePath:containingDirPath];
	
	// Ask the parent for a list of visible nodes so we can get at a FileSystemBrowserNode for the cell being displayed.
	// Then give the FileSystemBrowserNode to the cell so it can determine how to display itself.
	directoryContents = [containingDirNode visibleSubNodes];
	displayedCellNode = [directoryContents objectAtIndex:row];
		
	[cell setAttributedStringValueFromFileSystemBrowserNode:displayedCellNode];
}

/* get the path for column column
 */
- (NSString *)pathToColumn:(int)column
{
	NSString *path = nil;
	if (column == 0)
		path = @"/";
	else
		path = [browser pathToColumn:column];
	return path;
}

/* returns YES if the last selected item is a directory
 */
- (BOOL)isDirectory
{
	FileSystemBrowserNode *node = [FileSystemBrowserNode nodeWithParent:nil atRelativePath:[browser path]];
	return [node isDirectory];
}

/* returns YES if the given path is a directory
 */
- (BOOL)isDirectory:(NSString *)filename
{
	FileSystemBrowserNode *node = [FileSystemBrowserNode nodeWithParent:nil atRelativePath:filename];
	return [node isDirectory];
}

/* returns YES if any of the selected items are directories, else NO
 */
- (BOOL)directorySelected
{
	NSArray *selectedCells = [browser selectedCells];
	NSEnumerator *enumerator = [selectedCells objectEnumerator];
	NSBrowserCell *currentCell;
	
	NSString *basePath = [[self pathToColumn:[browser selectedColumn]] stringByAppendingString:@"/"];
	FileSystemBrowserNode *node;
	while (currentCell = [enumerator nextObject])
	{
		node = [FileSystemBrowserNode nodeWithParent:nil atRelativePath:[basePath stringByAppendingString:[currentCell stringValue]]];
		if ([node isDirectory])
			return YES;
	}
	return NO;
}

/* get all the selected files, recurse if recursion == YES
 */
- (NSArray *)selectedFilesWithRecursion:(BOOL)recursion
{
	NSArray *selectedCells = [browser selectedCells];
	NSMutableArray *selectedPaths = [[NSMutableArray alloc] init];
	NSEnumerator *enumerator = [selectedCells objectEnumerator];
	NSBrowserCell *currentCell;
	
	NSString *basePath = [[self pathToColumn:[browser selectedColumn]] stringByAppendingString:@"/"];
	FileSystemBrowserNode *node;
	while (currentCell = [enumerator nextObject])
	{
		node = [FileSystemBrowserNode nodeWithParent:nil atRelativePath:[basePath stringByAppendingString:[currentCell stringValue]]];
		if (recursion)
			[self addContentsOfNode:node toArray:selectedPaths];
		else if (![node isDirectory])
			[selectedPaths addObject:[node absolutePath]];
	}
	NSArray *selectedPathsImmutable = [NSArray arrayWithArray:selectedPaths];
	[selectedPaths release];
	return selectedPathsImmutable;
}

/* add all the files in the node node to 
 * the array array
 */
- (void)addContentsOfNode:(FileSystemBrowserNode *)node toArray:(NSMutableArray *)array
{
	if (![node isDirectory])
	{
		[array addObject:[node absolutePath]];
		
		return;
	}
	NSArray *subNodes = [node visibleSubNodes];
	NSEnumerator *enumerator = [subNodes objectEnumerator];
	FileSystemBrowserNode *currentNode;
	while (currentNode = [enumerator nextObject])
	{
		[self addContentsOfNode:currentNode toArray:array];
	}
}

@end
