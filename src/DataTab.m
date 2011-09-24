//
//  DataTab.m
//  XNJB
//

/* this class is for the data tab
 * copies files to/from NJB, deletes
 * does not support timestamp as this didn't
 * appear to work on my NJB3
 */

#import "DataTab.h"
#import "defs.h"
#include <sys/stat.h>
#import "ID3Tagger.h"

@interface DataTab (PrivateAPI)
- (NSString *)pathToColumn:(int)column;
- (void)showNewDir;
- (NSString *)makeNJBPathString:(NSString *)path;
- (void)addCreateDirToQueue:(NSString *)dirName;
- (void)createDir:(NSString *)dirName;
- (void)copyFromNJBDirectory:(Directory *)dir toPath:(NSString *)path;
- (void)deleteDir:(Directory *)dir fromParent:(Directory *)parent;
@end

// todo: implement drag and drop into the njbBrowser

@implementation DataTab

- (void)dealloc
{
	[baseDir release];
	[tempBaseDir release];
	[super dealloc];
}

- (void)onConnectAndActive
{
	[super onConnectAndActive];
	[self loadFiles];
}

- (NSString *)browserPathFromPreferences
{
	return [preferences dataTabDir];
}

/* add downloadFileList to the NJB queue
 */
- (void)loadFiles
{
	NJBQueueItem *getFiles = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(downloadFileList)];
	[getFiles setStatus:STATUS_DOWNLOADING_FILE_LIST];
	
	NJBQueueItem *updateTable = [[NJBQueueItem alloc] initWithTarget:self
																											withSelector:@selector(showNewDir)
																												withObject:nil
																							 withRunInMainThread:YES];
	[updateTable setDisplayStatus:NO];
	
	NSString *description = NSLocalizedString(@"Getting file list", nil);
	[self addToQueue:getFiles withSubItems:[NSArray arrayWithObjects:updateTable, nil] withDescription:description];
	[getFiles release];
	[updateTable release];
}

/* this is called from the queue consumer 
 * to get the files off the NJB and put them in tempArray
 * will run in worker thread
 */
- (NJBTransactionResult *)downloadFileList
{
	[tempBaseDir release];
	tempBaseDir = [theNJB dataFiles];
	[tempBaseDir retain];
	
	NJBTransactionResult *result = [[NJBTransactionResult alloc] initWithSuccess:(tempBaseDir != nil)];
	
	return [result autorelease];
}

/* get a list of all the files to copy from the NJB
*/
- (IBAction)copyFromNJB:(id)sender
{
	NSString *destDir = [NSString stringWithFormat:@"%@/", [browser directory]];

	NSArray *pathComponents = [njbBrowser containingPathComponents];
	// if a directory is selected, then we need to move one level back
	if (![[njbBrowser selectedCell] isLeaf])
	{
		NSMutableArray *temp = [NSMutableArray arrayWithArray:pathComponents];
		[temp removeLastObject];
		pathComponents = [NSArray arrayWithArray:temp];
	}
	Directory *containingDir = [baseDir itemWithPath:pathComponents];
	
	NSEnumerator *enumerator = [[njbBrowser selectedCells] objectEnumerator];
	NSBrowserCell *currentCell;
	MyItem *itemToCopy;
	while (currentCell = [enumerator nextObject])
	{
		itemToCopy = [containingDir itemWithDescription:[currentCell stringValue]];
		if ([itemToCopy isMemberOfClass:[Directory class]])
		{
			Directory *dir = (Directory *)itemToCopy;
			[self copyFromNJBDirectory:dir toPath:destDir];
		}
		else
		{
			DataFile *file = (DataFile *)itemToCopy;
			[file setFullPath:[destDir stringByAppendingString:[self replaceBadFilenameChars:[file filename]]]];
			[self copyFileFromNJB:file];
		}
	}
}

- (void)copyFromNJBDirectory:(Directory *)dir toPath:(NSString *)path
{
	path = [NSString stringWithFormat:@"%@%@/", path, [dir name]];
	
	[self addCreateDirToQueue:path];
	
	NSEnumerator *objectEnumerator = [dir objectEnumerator];
	MyItem *item;
	while (item = [objectEnumerator nextObject])
	{
		if ([item isMemberOfClass:[DataFile class]])
		{
			DataFile *file = (DataFile *)item;
			[file setFullPath:[path stringByAppendingString:[self replaceBadFilenameChars:[file filename]]]];
			[self copyFileFromNJB:file];
		}
		else
		{
			Directory *dir = (Directory *)item;
			[self copyFromNJBDirectory:dir toPath:path];
		}
	}
}

/* copy a file from the NJB at index index
 * in the itemArrayDisplaying
 */
- (void)copyFileFromNJB:(MyItem *)item
{
	DataFile *dataFile = (DataFile *)item;
	
	[self copyDataFileFromNJB:dataFile];
}

/* add the datafile dataFile to the queue
 * to be copied
 */
- (void)copyDataFileFromNJB:(DataFile *)dataFile
{
	if (![theNJB isConnected])
		return;
	NJBQueueItem *copyFile = [[NJBQueueItem alloc] initWithTarget:theNJB withSelector:@selector(downloadFile:) withObject:dataFile];
	[copyFile setStatus:STATUS_DOWNLOADING_FILE];
	NSString *description = [NSString stringWithFormat:NSLocalizedString(@"Downloading file '%@'", nil), [dataFile filename]];
	[self addToQueue:copyFile withSubItems:nil withDescription:description];
	[copyFile release];
	
	// could update browser here, but causes browser to lose selection so annoying
}

/* create the directory dirName (add to the queue)
 */
- (void)addCreateDirToQueue:(NSString *)dirName
{
	NJBQueueItem *createDir = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(createDir:) withObject:dirName];
	[createDir setDisplayStatus:NO];
	[self addToQueue:createDir];
	[createDir release];
}

- (void)createDir:(NSString *)dirName
{
	mkdir([dirName UTF8String], 0755);
}

/* copy a file to the NJB
 */
- (void)copyFileToNJB:(NSString *)filename
{
	MyItem *dataFile = [DataFile dataFileFromPath:filename];
	if (dataFile == nil)
	{
		// could not open, complain
		return;
	}
  
  if ([theNJB isAudioType:[dataFile fileType]])
  {
    ID3Tagger *tagger = [[ID3Tagger alloc] init];
    [tagger setFilename:[dataFile fullPath]];
    dataFile = [tagger readTrack];
    [tagger release];
  }
  
	[self copyDataFileToNJB:dataFile];
}

/* add the datafile dataFile to the queue
 * to be uploaded
 */
- (void)copyDataFileToNJB:(MyItem *)dataFile
{
	// todo: stop duplicate files being copied and confusing us
	if (![theNJB isConnected])
		return;
	
	NSString *path = [fileBrowser pathToColumn:[fileBrowser selectedColumn]];
	
	NSArray *destDirPathComponents;
	NSString *destDir = @"";
	
	destDir = [NSString stringWithString:[dataFile fullPath]];
	destDir = [destDir substringFromIndex:[path length]+1];
	destDir = [NSString stringWithFormat:@"%@%@", [njbBrowser containingPath], destDir];
	destDir = [destDir stringByDeletingLastPathComponent];
	// destDir now doesn't end in a /
		
	// so we need to remove just the first object from here
	destDirPathComponents = [destDir pathComponents];
	if ([destDirPathComponents count] > 0)
	{
		NSMutableArray *mutableComponents = [NSMutableArray arrayWithArray:destDirPathComponents];
		[mutableComponents removeObjectAtIndex:0];
		destDirPathComponents = [NSArray arrayWithArray:mutableComponents];
	}
	
  NJBQueueItem *copyFile = nil;
  if ([theNJB isAudioType:[dataFile fileType]])
  {
    copyFile = [[NJBQueueItem alloc] initWithTarget:theNJB withSelector:@selector(uploadTrack:) withObject:dataFile];
    [copyFile setStatus:STATUS_UPLOADING_FILE];
  }
  else
  {
    copyFile = [[NJBQueueItem alloc] initWithTarget:theNJB withSelector:@selector(uploadFile:toFolder:) withObject:dataFile];
    [copyFile setObject2:[self makeNJBPathString:destDir]];
    [copyFile setStatus:STATUS_UPLOADING_FILE];
  }
		
	NJBQueueItem *getUpdatedFileList = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(downloadFileList)];
	[getUpdatedFileList setDisplayStatus:NO];
	
	NJBQueueItem *updateTable = [[NJBQueueItem alloc] initWithTarget:self
																											withSelector:@selector(showNewDir)
																												withObject:nil
																							 withRunInMainThread:YES];
	[updateTable setDisplayStatus:NO];
		
	[copyFile cancelItemIfFail:getUpdatedFileList];
	[getUpdatedFileList cancelItemIfFail:updateTable];
	
	NSString *description = [NSString stringWithFormat:NSLocalizedString(@"Uploading file '%@'", nil), [dataFile filename]];
	[self addToQueue:copyFile withSubItems:[NSArray arrayWithObjects:getUpdatedFileList, updateTable, nil] withDescription:description];
	
	[copyFile release];
	[getUpdatedFileList release];
	[updateTable release];
}

/* delete the item from the NJB
 */
- (void)deleteFromNJB:(MyItem *)item
{
	[self deleteFromNJB:item fromParent:nil];
}

/* delete the item from the NJB
*/
- (void)deleteFromNJB:(MyItem *)item fromParent:(Directory *)parentDir
{	
	if (parentDir == nil)
	{
		NSLog(@"parentDir == nil in deleteFromNJB:fromParent! Not updating cache so expect problems\n");
	}
	
	NJBQueueItem *getUpdatedFileList = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(downloadFileList)];
	[getUpdatedFileList setDisplayStatus:NO];
	
	NJBQueueItem *updateTable = [[NJBQueueItem alloc] initWithTarget:self
																											withSelector:@selector(showNewDir)
																												withObject:nil
																							 withRunInMainThread:YES];
	[updateTable setDisplayStatus:NO];
	
	NJBQueueItem *deleteFile = [[NJBQueueItem alloc] initWithTarget:theNJB withSelector:@selector(deleteFile:fromDir:)
																											 withObject:item];
	[deleteFile setObject2:parentDir];
	[deleteFile setStatus:STATUS_DELETING_FILE];
	
	[deleteFile cancelItemIfFail:getUpdatedFileList];
	[getUpdatedFileList cancelItemIfFail:updateTable];
	
	NSString *description = [NSString stringWithFormat:NSLocalizedString(@"Deleting file '%@'", nil), [item description]];
	[self addToQueue:deleteFile withSubItems:[NSArray arrayWithObjects:getUpdatedFileList, updateTable, nil] withDescription:description];
		
	[deleteFile release];
	[getUpdatedFileList release];
	[updateTable release];
}

- (void)fileSystemBrowserFileClick:(NSString *)path
{
	[self setActiveObject:OBJECT_FILE_BROWSER];
	[super fileSystemBrowserFileClick:path];
	DataFile *dataFile = [DataFile dataFileFromPath:path];
	if (dataFile != nil)
		[drawerController showDataFile:dataFile];
	else
	{
		// the file/dir has been deleted, so reload
		// todo: how many shall we reload? user could have deleted deep directory structure
		unsigned int column = [fileBrowser lastColumn];
		[fileBrowser reloadColumn:column];
		if (column != 0)
			[fileBrowser reloadColumn:column - 1];
		[self enableTransfers];
	}
}

- (void)fileSystemBrowserDirectoryClick:(NSString *)path
{
	[self setActiveObject:OBJECT_FILE_BROWSER];
	[super fileSystemBrowserDirectoryClick:path];
	[drawerController disableAll];
}

- (void)activate
{
	[super activate];
	if (activeObject == OBJECT_FILE_BROWSER)
	{
		if (![browser isDirectory])
		{
			DataFile *dataFile = [DataFile dataFileFromPath:[browser path]];
			if (dataFile != nil)
				[drawerController showDataFile:dataFile];
		}
	}
}

- (void)applicationTerminating:(NSNotification *)note
{
	[preferences setLastUsedDataTabDir:[self browserDirectory]];
	[super applicationTerminating:note];
}

- (void)showDrawerInfo
{
	id item = [baseDir itemWithPath:[njbBrowser pathComponents]];
	
	if (item != nil && [item isMemberOfClass:[DataFile class]])
	{
		[drawerController showDataFile:(DataFile *)item];
	}
	else
	{
		[drawerController clearAll];
		[drawerController disableAll];
	}
}

- (int)browser:(NSBrowser *)sender numberOfRowsInColumn:(int)column
{
	return [[baseDir itemWithPath:[njbBrowser pathComponents]] itemCount];
}

- (void)browser:(NSBrowser *)sender willDisplayCell:(id)cell atRow:(int)row column:(int)column
{
	Directory *dir = [baseDir itemWithPath:[njbBrowser pathComponentsToColumn:column]];
	id item = [dir itemAtIndex:row];
	
	BOOL isDir = [item isMemberOfClass:[Directory class]];
	[cell setLeaf:!isDir];
	[cell setStringValue:[item description]];
}

- (IBAction)browserSingleClick:(id)sender
{
	[self setActiveObject:OBJECT_NJB_TABLE];
	[self showDrawerInfo];
	[self enableTransfers];
}

- (void)showNewDir
{
	[baseDir release];
	baseDir = tempBaseDir;
	tempBaseDir = nil;
	[self reloadNJBDirs];
}

- (IBAction)newFolder:(id)sender
{
	NSString *newFolderName = [newFolderText stringValue];
	Directory *dir = [[Directory alloc] initWithName:newFolderName];
	NSString *parentDir = [self makeNJBPathString:[njbBrowser containingPath]];
 	NJBQueueItem *createFolder = nil;
	
	createFolder = [[NJBQueueItem alloc] initWithTarget:theNJB withSelector:@selector(createFolder:inDir:)
																													 withObject:dir];
	[createFolder setObject2:parentDir];
 	[createFolder setStatus:STATUS_CREATING_FOLDER];
	
	NJBQueueItem *getUpdatedFileList = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(downloadFileList)];
	[getUpdatedFileList setDisplayStatus:NO];
	
	NJBQueueItem *updateTable = [[NJBQueueItem alloc] initWithTarget:self
																											withSelector:@selector(showNewDir)
																												withObject:nil
																							 withRunInMainThread:YES];
	[updateTable setDisplayStatus:NO];
		
	[createFolder cancelItemIfFail:getUpdatedFileList];
	[getUpdatedFileList cancelItemIfFail:updateTable];
	
	NSString *description = [NSString stringWithFormat:NSLocalizedString(@"Creating folder '%@'", nil), newFolderName];
 	[self addToQueue:createFolder withSubItems:[NSArray arrayWithObjects:getUpdatedFileList, updateTable, nil] withDescription:description];

	[createFolder release];
	[getUpdatedFileList release];
	[updateTable release];
	[dir release];
}

- (void)NJBDisconnected:(NSNotification *)note
{
	[newFolderButton setEnabled:NO];
	
	[super NJBDisconnected:note];
}

- (void)NJBConnected:(NSNotification *)note
{
	[baseDir release];
	baseDir = nil;
	[self reloadNJBDirs];
	
	[newFolderButton setEnabled:[self canCreateFolder]];
	
	[super NJBConnected:note];
}


- (NSString *)makeNJBPathString:(NSString *)path
{
	NSMutableString *temp = [NSMutableString stringWithString:path];
	[temp replaceOccurrencesOfString:@"/" withString:@"\\" options:0 range:NSMakeRange(0, [temp length])];
	if ([temp length] == 0 || [temp characterAtIndex:[temp length] - 1] != '\\')
		[temp appendString:@"\\"];
	return [NSString stringWithString:temp];
}

// override this - we do it ourselves on clicks
- (void)onFirstResponderChange:(NSResponder *)aResponder
{}

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
	
	// get the base path components - this is the right most directory
	// so if the last selected item was a directory, we need to remove that
	NSArray *basePathComponents = [njbBrowser containingPathComponents];
	// we will get 0 count if the item has no name
	if ([basePathComponents count] > 0 && ![[njbBrowser selectedCell] isLeaf])
	{
		NSMutableArray *temp = [NSMutableArray arrayWithArray:basePathComponents];
		[temp removeLastObject];
		basePathComponents = [NSArray arrayWithArray:temp];
	}

	Directory *containingDir = [baseDir itemWithPath:basePathComponents];
	NSArray *selectedCells = [njbBrowser selectedCells];
	NSEnumerator *enumerator = [selectedCells objectEnumerator];
	NSBrowserCell *currentCell;
	MyItem *itemToDelete;
	while (currentCell = [enumerator nextObject])
	{
		itemToDelete = [containingDir itemWithDescription:[currentCell stringValue]];
		if ([itemToDelete isMemberOfClass:[Directory class]])
			[self deleteDir:(Directory *)itemToDelete fromParent:containingDir];
		else
			[self deleteFromNJB:itemToDelete fromParent:containingDir];
	}
}

// deletes the directory and all files/directories contained in it
- (void)deleteDir:(Directory *)dir fromParent:(Directory *)parent
{
	NSEnumerator *objectEnumerator = [dir objectEnumerator];
	MyItem *toDelete = nil;
	while (toDelete = [objectEnumerator nextObject])
	{
		if ([toDelete isMemberOfClass:[Directory class]])
			[self deleteDir:(Directory *)toDelete fromParent:dir];
		else
			[self deleteFromNJB:toDelete fromParent:dir];
	}
	[self deleteFromNJB:dir fromParent:parent];
}

- (void)reloadNJBDirs
{
	[njbBrowser reloadColumn:[njbBrowser lastColumn]];
	// reload the previous column too - we might have changed the directory containing the right most files
	if ([njbBrowser lastColumn] != 0)
		[njbBrowser reloadColumn:[njbBrowser lastColumn] - 1];
	
	[self enableTransfers];
}

/* can we delete something? (override from MyFileTab)
*/
- (BOOL)canDelete
{
	switch ([self activeObject])
	{
		case OBJECT_NJB_TABLE:
			if (![theNJB isConnected])
				return NO;
			if ([njbBrowser selectedCell] != nil)
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

/* can we copy a file from the NJB? (override from MyFileTab)
*/
- (BOOL)canCopyFromNJB
{
	if (![theNJB isConnected])
		return NO;
	else
		return ([njbBrowser selectedCell] != nil);
}

- (BOOL)canCreateFolder
{
	if (![theNJB isConnected])
		return NO;
	if (![theNJB isProtocol3Device] && ![theNJB isMTPDevice])
		return NO;
	if ([[newFolderText stringValue] length] > 0)
		return YES;
	else
		return NO;
}

// the newFolderText text changed
- (void)controlTextDidChange:(NSNotification *)aNotification
{
	[newFolderButton setEnabled:[self canCreateFolder]];
}

@end
