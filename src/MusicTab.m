//
//  MusicTab.m
//  XNJB
//

/* this tab is for the music tab
 * upload/download tracks, delete
 */

#import "MusicTab.h"
#import "Track.h"
#import "ID3Tagger.h"
#import "defs.h"
// for stat
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// these correspond to the identifiers in the NIB file
#define COLUMN_TITLE @"Title"
#define COLUMN_ARTIST @"Artist"
#define COLUMN_ALBUM @"Album"
#define COLUMN_GENRE @"Genre"
#define COLUMN_LENGTH @"Length"
#define COLUMN_TRACK_NO @"Track No"
#define COLUMN_CODEC @"Codec"
#define COLUMN_FILESIZE @"Filesize"
#define COLUMN_YEAR @"Year"

// declare the private methods
@interface MusicTab (PrivateAPI)
- (NSString *)filenameForTrack:(Track *)track;
- (void)loadiTunesLibrary;
@end

@implementation MusicTab

/**********************/

- (void)awakeFromNib
{
	[super awakeFromNib];
	
	[self setCorrectView];		
}

- (NSString *)browserPathFromPreferences
{
	return [preferences musicTabDir];
}

- (void)onConnectAndActive
{
	[super onConnectAndActive];
	if (!trackListUpToDate)
		[self loadTracks];
}

/* replaces track variables (%t, %a, ...)
 * with actual values for the track given.
 * does not add extension
 * called when copying file from NJB
 */
- (NSString *)filenameForTrack:(Track *)track
{
	// ignores [track filename] as this is always blank on my NJB3
	NSMutableString *filename = [[NSMutableString alloc] init];
	
	NSString *filenameFormat = [preferences filenameFormat];
	int i = -1;
	BOOL gotPercent = NO;
	for (i = 0; i < [filenameFormat length]; i++)
	{
		unichar currentChar = [filenameFormat characterAtIndex:i];
		if (gotPercent)
		{
			gotPercent = NO;
			switch (currentChar)
			{
				case '%':
					[filename appendString:@"%"];
					break;
					// title
				case 't':
					[filename appendString:[self replaceBadFilenameChars:[track title]]];
					break;
					// artist
				case 'a':
					[filename appendString:[self replaceBadFilenameChars:[track artist]]];
					break;
					// album
				case 'A':
					[filename appendString:[self replaceBadFilenameChars:[track album]]];
					break;
					// genre
				case 'g':
					[filename appendString:[self replaceBadFilenameChars:[track genre]]];
					break;
					// track no
				case 'n':
          if ([track trackNumber] <= 9)
            [filename appendString:@"0"];
					[filename appendString:[NSString stringWithFormat:@"%d",[track trackNumber]]];
					break;
					// codec
				case 'c':
					[filename appendString:[track fileTypeString]];
					break;
				default:
					// incorrect variable name so print nothing
					break;
			}
		}
		else if (currentChar == '%')
			gotPercent = YES;
		else
			[filename appendString:[NSString stringWithCharacters:&currentChar length:1]];
	}
	NSString *f = [NSString stringWithString:filename];
	[filename release];
	return f;
}

/* copy the item at index in itemArrayDisplaying
 * from the NJB
 */
- (void)copyFileFromNJB:(MyItem *)item
{
	Track *track = (Track *)item;
	NSFileManager *manager = [NSFileManager defaultManager];
	
	NSString *filename = [self filenameForTrack:track];
	NSString *directory = nil;
	if ([preferences iTunesIntegration])
		directory = [[[preferences downloadDir] stringByExpandingTildeInPath] stringByAppendingString:@"/"];
	else
		directory = [[browser directory] stringByAppendingString:@"/"];
	
	NSString *fullPath = [directory stringByAppendingString:filename];
	
	NSArray *components = [fullPath pathComponents];
	NSMutableString *dirsSoFar = [[NSMutableString alloc] initWithString:@"/"];
	int i = -1;
	for (i = 0; i < [components count]; i++)
	{
		NSString *component = [components objectAtIndex:i];
		
		if ([component isEqual:@"/"])
		{
			// the format string starts with /, this will get ignored when prefixing it with directory
			// so don't do anything now
		}
		else if (i != [components count] - 1)
		{
			// this is not the filename, need to make a directory
			[dirsSoFar appendString:component];
			[dirsSoFar appendString:@"/"];
			// create the directory, will fail if already made, don't worry
			[manager createDirectoryAtPath:dirsSoFar attributes:nil];
		}
	}
	
	NSString *extension = [[NSString stringWithString:@"."] stringByAppendingString:[[track fileTypeString] lowercaseString]];
	
	fullPath = [fullPath stringByAppendingString:extension];
	
	[track setFullPath:fullPath];
		
	[self copyTrackFromNJB:track];
}

/* actually add the track to the queue
 * to be copied
 */
- (void)copyTrackFromNJB:(Track *)track
{
	if (![theNJB isConnected])
		return;
		
	NJBQueueItem *copyTrack = [[NJBQueueItem alloc] initWithTarget:theNJB withSelector:@selector(downloadTrack:) withObject:track];
	[copyTrack setStatus:STATUS_DOWNLOADING_TRACK];
	NSString *description = [NSString stringWithFormat:NSLocalizedString(@"Downloading track '%@'", nil), [track title]];
	
	NSMutableArray *tasks = [[NSMutableArray alloc] initWithCapacity:2];
	
	// write track tag to file
	if ([preferences writeTagAfterCopy] && [track fileType] == LIBMTP_FILETYPE_MP3)
	{
		NJBQueueItem *writeTag = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(writeTagToFile:) withObject:track];
		[writeTag setRunInMainThread:YES];
		[writeTag setDisplayStatus:NO];
		
		[tasks addObject:writeTag];
    
    [writeTag release];
	}
	
	if ([preferences iTunesIntegration])
	{
		NJBQueueItem *addToiTunes = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(addTrackToiTunes:) withObject:track];
		[addToiTunes setRunInMainThread:YES];
		[addToiTunes setDisplayStatus:NO];
		
		[tasks addObject:addToiTunes];
		
		[addToiTunes release];
	}
	
	[self addToQueue:copyTrack withSubItems:tasks withDescription:description];
	
	[copyTrack release];
	[tasks release];
}

/* copy the file at filename to
 * the NJB.
 */
- (void)copyFileToNJB:(NSString *)filename
{
  Track *track = [[Track alloc] init];
  [track setFullPath:filename];
  [track fileTypeFromExtension];
  
	[self copyTrackToNJB:track];
  
	[track release];
}

/* actually add the track to the queue
 * to be copied
 */
- (void)copyTrackToNJB:(Track *)track
{
	if (![theNJB isConnected])
		return;
	
	/*[track fileTypeFromExtension];
	if ([theNJB isSupportedFileType*/
	
	NJBQueueItem *copyTrack = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(uploadTrack:) withObject:track];
	[copyTrack setStatus:STATUS_UPLOADING_TRACK];
	
	NJBQueueItem *addToList = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(addToArray:) withObject:track withRunInMainThread:YES];
	[addToList setDisplayStatus:NO];
	
	[copyTrack cancelItemIfFail:addToList];
	
	NSString *description = [NSString stringWithFormat:NSLocalizedString(@"Uploading track '%@'", nil), [track fullPath]];
	[self addToQueue:copyTrack withSubItems:[NSArray arrayWithObjects:addToList, nil] withDescription:description];
	
	[copyTrack release];
	[addToList release];
}

/* uploads the track to the jukebox, run in worker thread
 */
- (NJBTransactionResult *)uploadTrack:(Track *)track
{
  // assume we have no tag if title is empty - catch drag&drop tracks here
	if (![preferences iTunesIntegration] || [[track title] length] == 0)
	{
		ID3Tagger *tagger = [[ID3Tagger alloc] init];
		[tagger setFilename:[track fullPath]];
		Track *trackWithTag = [tagger readTrack];
		[track setValuesToTrack:trackWithTag];
		[tagger release];
	}
  
  // don't trust old filesize
  
  struct stat sb;
	// check file exists
	if (stat([[track fullPath] fileSystemRepresentation], &sb) == -1)
		return [[NJBTransactionResult alloc] initWithSuccess:NO resultString:NSLocalizedString(@"File no longer exists", nil)];

  [track setFilesize:sb.st_size];
	
	return [theNJB uploadTrack:track];
}

/* write the info in track to the tag in the file
 */
- (void)writeTagToFile:(Track *)track
{
	ID3Tagger *tagger = [[ID3Tagger alloc] init];
	[tagger writeTrack:track];
	[tagger release];
}

/* update the drawer when someone selects a different
 * track
 */
- (void)tableViewSelectionDidChange:(NSNotification *)aNotification
{
	// we might be called from code changing the selection but still update the
	// item count anyway since it could be the iTunes table
	if (activeObject != OBJECT_NJB_TABLE)
	{
		[self enableTransfers];
		[self showItemCounts];
    
    if ([aNotification object] == iTunesTable)
      [self showDrawerInfo];
    
		return;
	}
	[super tableViewSelectionDidChange:aNotification];
	// unchanged tag edit will be lost
	
	if ([njbTable selectedRow] != -1)
		objectToSaveTrack = OBJECT_NJB_TABLE;
}

/* called by MainController when we must write
 * a track to a file/njb
 */
- (void)drawerWriteTag:(Track *)modifiedTrack
{
	switch (objectToSaveTrack)
	{
		case OBJECT_NJB_TABLE:
			// none selected, shouldn't be here anyway
			if ([njbTable selectedRow] == -1)
				return;	
			
			Track *oldTrack = (Track *)[[arrayController arrangedObjects] objectAtIndex:[njbTable selectedRow]];
			
			NJBQueueItem *changeTrackTag = [[NJBQueueItem alloc] initWithTarget:theNJB withSelector:@selector(changeTrackTagTo:from:) withObject:modifiedTrack];
			[changeTrackTag setObject2:oldTrack];
			[changeTrackTag setStatus:STATUS_UPDATING_TRACK_TAG];
			
			NJBQueueItem *updateTrackList = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(replaceTrack:withTrack:)
																																withObject:oldTrack withRunInMainThread:YES];
			[updateTrackList setObject2:modifiedTrack];
			[updateTrackList setDisplayStatus:NO];
			
			[changeTrackTag cancelItemIfFail:updateTrackList];
			
			NSString *description = [NSString stringWithFormat:NSLocalizedString(@"Changing tag for track '%@'", nil), [oldTrack title]];
			[self addToQueue:changeTrackTag withSubItems:[NSArray arrayWithObjects:updateTrackList, nil] withDescription:description];

			[changeTrackTag release];
			[updateTrackList release];
			
			break;
		case OBJECT_FILE_BROWSER:
			[self writeTagToFile:modifiedTrack];
			break;
		default:
			NSLog(@"Drawer write tag without njbTable/fileBrowser selected!");
	}
}

- (void)fileSystemBrowserFileClick:(NSString *)path
{
	[super fileSystemBrowserFileClick:path];
	objectToSaveTrack = OBJECT_FILE_BROWSER;
	// unchanged tag edit will be lost
	ID3Tagger *tagger = [[ID3Tagger alloc] initWithFilename:path];
	[drawerController enableAll];
	Track *track = [tagger readTrack];
	[drawerController showTrack:track];
	[tagger release];
	if ([track fileType] != LIBMTP_FILETYPE_MP3)
		[drawerController disableWrite];
}

- (void)fileSystemBrowserDirectoryClick:(NSString *)path
{
	[super fileSystemBrowserDirectoryClick:path];
	// unchanged tag edit will be lost
	[drawerController disableAll];
}

- (void)onFirstResponderChange:(NSResponder *)aResponder
{
	[super onFirstResponderChange:(NSResponder *)aResponder];
	if (aResponder == njbTable)
	{
		objectToSaveTrack = OBJECT_NJB_TABLE;
	}
	else if ([aResponder class] == [NSMatrix class])
		objectToSaveTrack = OBJECT_FILE_BROWSER;
	// used to be [aResponder class] == [NSSearchField class]
	else if (aResponder == njbSearchField)
	{
		[drawerController disableAll];
	}
}

/* add downloadTrackList to the njb queue
 */
- (void)loadTracks
{
	if (trackListUpToDate)
	{
		NSLog(@"loadTracks called in Music tab when track list up to date");
		return;
	}
	NSMutableArray *cachedTrackList = [theNJB cachedTrackList];
	if (cachedTrackList != nil)
	{
		[tempArray release];
		tempArray = cachedTrackList;
		[tempArray retain];
		[self showNewArray];
		
		trackListUpToDate = YES;
		return;
	}
	
	NJBQueueItem *getTracks = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(downloadTrackList)];
	[getTracks setStatus:STATUS_DOWNLOADING_TRACK_LIST];

	NJBQueueItem *updateTable = [[NJBQueueItem alloc] initWithTarget:self
																											withSelector:@selector(showNewArray)
																												withObject:nil
																							 withRunInMainThread:YES];
	[updateTable setDisplayStatus:NO];
	
	[getTracks cancelItemIfFail:updateTable];
	
	NSString *description = NSLocalizedString(@"Downloading Track List", nil);
	[self addToQueue:getTracks withSubItems:[NSArray arrayWithObjects:updateTable, nil] withDescription:description];
	[getTracks release];
	[updateTable release];
	// it's not actually yet, but will be in a while, don't want to go copying anything
	trackListUpToDate = YES;
}

/* this is called from the queue consumer 
 * to get the tracks off the NJB and put them in fullItemArray
 * will run in worker thread
 */
- (NJBTransactionResult *)downloadTrackList
{
	[tempArray release];
	tempArray = [theNJB tracks];
	[tempArray retain];
	
	NJBTransactionResult *result = [[NJBTransactionResult alloc] initWithSuccess:(tempArray != nil)];
	
	trackListUpToDate = YES;
	
	return [result autorelease];
}

- (void)deleteFromNJB:(MyItem *)item
{
	Track *track = (Track *)item;

	NJBQueueItem *deleteTrack = [[NJBQueueItem alloc] initWithTarget:theNJB withSelector:@selector(deleteTrack:)
																												withObject:track];
	[deleteTrack setStatus:STATUS_DELETING_TRACK];
		
	NJBQueueItem *removeFromAllTracks = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(removeFromArray:)
																																	withObject:track];
	[removeFromAllTracks setRunInMainThread:YES];
	[removeFromAllTracks setDisplayStatus:NO];
	
	[deleteTrack cancelItemIfFail:removeFromAllTracks];
	
	NSString *description = [NSString stringWithFormat:NSLocalizedString(@"Deleting track '%@'", nil), [track title]];
	[self addToQueue:deleteTrack withSubItems:[NSArray arrayWithObjects:removeFromAllTracks, nil] withDescription:description];

	[deleteTrack release];
	[removeFromAllTracks release];
}

- (void)replaceTrack:(Track *)replace withTrack:(Track *)new
{
	[arrayController replaceObject:replace withObject:new];
//	[arrayController rearrangeObjects];
}

- (void)NJBConnected:(NSNotification *)note
{
	trackListUpToDate = NO;
	
	[super NJBConnected:note];
}

- (void)NJBDisconnected:(NSNotification *)note
{
	trackListUpToDate = NO;
	
	[super NJBDisconnected:note];
	
	if (isActive && activeObject == OBJECT_NJB_TABLE)
		[drawerController disableWrite];
}

- (void)activate
{	
	[super activate];
	if ([theNJB isConnected] && !trackListUpToDate)
	{
		[self loadTracks];
	}
	else if (activeObject == OBJECT_FILE_BROWSER)
	{
		if (![browser isDirectory])
		{
			ID3Tagger *tagger = [[ID3Tagger alloc] initWithFilename:[browser path]];
			[drawerController enableAll];
			[drawerController showTrack:[tagger readTrack]];
			[tagger release];
		}
	}
}

- (void)applicationTerminating:(NSNotification *)note
{
	[preferences setLastUsedMusicTabDir:[self browserDirectory]];
	[super applicationTerminating:note];
}

- (void)NJBTrackListModified:(NSNotification *)note
{
	if (!isActive)
	{
		trackListUpToDate = NO;
	}
}

- (void)showDrawerInfo
{
	// update display to show new track
  if (activeObject == OBJECT_NJB_TABLE && [njbTable selectedRow] > -1)
  {
    [drawerController showTrack:(Track *)[[arrayController arrangedObjects] objectAtIndex:[njbTable selectedRow]]];
		if (![theNJB isConnected])
			[drawerController disableWrite];
  }
  else if ([iTunesTable selectedRow] > -1)
  {
    [drawerController showTrack:(Track *)[[iTunesArrayController arrangedObjects] objectAtIndex:[iTunesTable selectedRow]]];
    [drawerController disableWrite];
  }
  else
  {
		[drawerController clearAll];
		[drawerController disableAll];
	}
}

- (void)loadiTunesLibrary
{
	[iTunesArrayController removeAll];
  
  NSString *iTunesDirectory = [[preferences iTunesDirectory] stringByExpandingTildeInPath];
	NSString *iTunesLibrary = [NSString stringWithFormat:@"%@/iTunes Music Library.xml", iTunesDirectory];
	NSDictionary *iTunesDict = [NSDictionary dictionaryWithContentsOfFile:iTunesLibrary];
	
	// if we made an error loading it then give up
	if (iTunesDict == nil)
	{
		NSRunAlertPanel(NSLocalizedString(@"iTunes Library", nil), 
										[NSString stringWithFormat:@"%@ '%@'", NSLocalizedString(@"Could not load iTunes library from", nil), iTunesLibrary],
										NSLocalizedString(@"OK", nil),
										nil, nil);
		return;
	}
  
  /* The album art is complicated here.  iTunes has a directory with itc files in
    iTunes/Album Artwork/.  The filenames are of the form libraryPersistentID-TrackPersistentID.itc.
    So here we go through the filesystem and map track IDs to filenames.  Then we go through all the tracks
    and find the itc filename for that track.  Note that the artwork is only stored for one track (I think
    normally track 1) so we have to build up another map from album name to itc.
  */
  
  // firstly make a dictionary of album art itc file locations
  NSString *libraryPersistentID = [iTunesDict objectForKey:@"Library Persistent ID"];
  NSMutableDictionary *itcFileDict = [[NSMutableDictionary alloc] init];
  NSArray *dirsToSearch = [NSArray arrayWithObjects:[NSString stringWithFormat:@"%@/Album Artwork/Local/%@", iTunesDirectory, libraryPersistentID],
    [NSString stringWithFormat:@"%@/Album Artwork/Downloaded/%@", iTunesDirectory, libraryPersistentID],
    [NSString stringWithFormat:@"%@/Album Artwork/Cache/%@", iTunesDirectory, libraryPersistentID], nil];
  NSEnumerator *baseDirEnum = [dirsToSearch objectEnumerator];
  NSString *baseDir = nil;
  while (baseDir = [baseDirEnum nextObject])
  {
    NSDirectoryEnumerator *dirEnum = [[NSFileManager defaultManager] enumeratorAtPath:baseDir];
    NSString *file;
    while (file = [dirEnum nextObject])
    {
      if ([[file pathExtension] isEqualToString: @"itc"])
      {
        //NSLog([NSString stringWithFormat:@"got itc file: %@", file]);
      
        // filename should be in the form of libraryid-itemid.itc
        NSArray *pathComponents = [[[file lastPathComponent] stringByDeletingPathExtension] componentsSeparatedByString:@"-"];
        if (![libraryPersistentID isEqualToString:[pathComponents objectAtIndex:0]])
          NSLog([NSString stringWithFormat:@"Invalid filename for iTunes albumart: %@", file]);
        else
        {
          //NSLog([NSString stringWithFormat:@"persistent id: %@", [pathComponents objectAtIndex:1]]);
      
          [itcFileDict setObject:[NSString stringWithFormat:@"%@/%@", baseDir, file] forKey:[pathComponents objectAtIndex:1]];
        }
      }
    }
  }

  // to store the map from album name to itc file
  NSMutableDictionary *itcFileDictFromAlbums = [[NSMutableDictionary alloc] init];
  
	// Now do the tracks
	NSDictionary *tracksDict = [iTunesDict objectForKey:@"Tracks"];
  
  // to store the array since we can't enumerate from array controller without sorting
  NSMutableArray *iTunesTracks = [[NSMutableArray alloc] init];
	
  NSFileManager *manager = [NSFileManager defaultManager];
	NSDictionary *thisTrackDict;
	NSArray *tracksDictArray = [tracksDict allValues];
	NSEnumerator *objectEnumerator = [tracksDictArray objectEnumerator];
	while (thisTrackDict = [objectEnumerator nextObject])
	{
		// note we can't use the Kind field of the dictionary because this is localized
		// so we must use the filename extension to determine codec
		 
		Track *track = [[Track alloc] init];
    NSString *location = [thisTrackDict valueForKey:@"Location"];
    if (location == nil)
    {
      NSLog(@"location nil when reading in iTunes library! Ignoring entry");
      [track release];
      continue;
    }
    
    [track setFullPath:[[NSURL URLWithString:location] relativePath]];
    
    if (![manager fileExistsAtPath:[track fullPath]])
    {
      NSNumber *trackId = [thisTrackDict valueForKey:@"Track ID"];
      NSLog(@"iTunes track with path '%@' and ID '%@' no longer exists", [track fullPath], [trackId stringValue]);
      [track release];
      continue;
    }
    
		[track fileTypeFromExtension];
		
		[track setTitle:[thisTrackDict valueForKey:@"Name"]];
		[track setAlbum:[thisTrackDict valueForKey:@"Album"]];
		[track setArtist:[thisTrackDict valueForKey:@"Artist"]];
		[track setGenre:[thisTrackDict valueForKey:@"Genre"]];
		[track setLength:([[thisTrackDict valueForKey:@"Total Time"] unsignedLongValue] / 1000)];
		[track setYear:[[thisTrackDict valueForKey:@"Year"] unsignedIntValue]];
		[track setTrackNumber:[[thisTrackDict valueForKey:@"Track Number"] unsignedIntValue]];
    
    // don't trust iTunes filesize - it might have changed
    NSDictionary *attributes = [manager fileAttributesAtPath:[track fullPath] traverseLink:YES];
    [track setFilesize:[[attributes objectForKey:NSFileSize] unsignedLongLongValue]];
    
    [track setDateAdded:[thisTrackDict valueForKey:@"Date Added"]];
    [track setRating:[[thisTrackDict valueForKey:@"Rating"] unsignedIntValue]];

    NSString *itcPath = [itcFileDict objectForKey:[thisTrackDict objectForKey:@"Persistent ID"]];
    if (itcPath != nil)
    {
      [itcFileDictFromAlbums setObject:itcPath forKey:[track album]];
      [track setItcFilePath:itcPath];
    }    
			
		[iTunesArrayController addObject:track];
    [iTunesTracks addObject:track];
		[track release];
	}
  
  // now fill in artwork from albums (can't do before since tracks may come in the wrong order)
  objectEnumerator = [iTunesTracks objectEnumerator];
  Track *curTrack = nil;
  while (curTrack = [objectEnumerator nextObject])
  {
    if ([[curTrack itcFilePath] isEqualToString:@""])
    {
      [curTrack setItcFilePath:[itcFileDictFromAlbums objectForKey:[curTrack album]]];
    }
  }
  
  [itcFileDict release];
  [itcFileDictFromAlbums release];
  [iTunesTracks release];
	
	[self showItemCounts];
}

/* get a list of all the files to copy to the NJB
 */
- (IBAction)copyToNJB:(id)sender
{
	if (![preferences iTunesIntegration])
		return [super copyToNJB:sender];
	
	NSArray *filesToCopy = [iTunesArrayController selectedObjects];
	Track *currentTrack;
	NSEnumerator *enumerator = [filesToCopy objectEnumerator];
	while (currentTrack = [enumerator nextObject])
	{
		[self copyTrackToNJB:currentTrack];
	}
}

- (void)addTrackToiTunes:(Track *)track
{
	NSString *code = [NSString stringWithFormat:@" \
	set filenameP to \"%@\"									\n \
	set filenameAS to POSIX file filenameP	\n \
	tell application \"iTunes\"							\n \
		launch																\n \
		set this_track to add filenameAS to playlist \"Library\" of source \"Library\" \n \
	end tell" \
		, [track fullPath]];
	
	NSAppleScript *appleScript = [[NSAppleScript alloc] initWithSource:code];
	NSDictionary *errorInfo;
	
	if (![appleScript compileAndReturnError:&errorInfo])
	{
		NSLog(@"Could not compile AppleScipt!  Code:\n%@\n\n, errorinfo: %@", code, [errorInfo description]);
	}
	
	if ([appleScript executeAndReturnError:&errorInfo] == nil)
	{
		NSLog(@"Could not execute AppleScipt!  Code:\n%@\n\n, errorinfo: %@", code, [errorInfo description]);
	}
	
	[appleScript release];
	
	// add it to the list
	[iTunesArrayController addObject:track];
	[self showItemCounts];
}

/* delegate method called when
 * the search box is modified
 */
- (IBAction)searchiTunesTableAction:(id)sender
{
	// iTunesSearchField has been changed
	[iTunesArrayController search:sender];
	
	[self showItemCounts];
}

- (void)showItemCounts
{
	if ([preferences iTunesIntegration])
	{
		NSString *selectedString = [NSString stringWithFormat:NSLocalizedString(@"Selected %d out of %d", nil), [iTunesTable numberOfSelectedRows], [[iTunesArrayController arrangedObjects] count]];
		[iTunesItemCountField setStringValue:selectedString];
	}
	
	[super showItemCounts];
}

/* sets the view with iTunes table or file browser
 * depending on settings
 */
- (void)setCorrectView
{
	if ([preferences iTunesIntegration])
	{
		[iTunesBox setHidden:NO];
		[iTunesSearchField setHidden:NO];
		[iTunesItemCountField setHidden:NO];
		[fileBrowser setHidden:YES];
		
		[self loadiTunesLibrary];
	}
	else
	{
		[iTunesBox setHidden:YES];
		[iTunesSearchField setHidden:YES];
		[iTunesItemCountField setHidden:YES];
		[fileBrowser setHidden:NO];
	}
}

/* can we copy a file to the NJB? We override super so we can test to see if
 * iTunes table has a selection
 */
- (BOOL)canCopyToNJB
{
	if (![theNJB isConnected])
		return NO;
	else if ([preferences iTunesIntegration])
		return ([iTunesTable selectedRow] != -1);
	else
		return [super canCopyToNJB];
}

@end