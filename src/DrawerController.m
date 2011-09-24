//
//  DrawerController.m
//  XNJB
//

/* controls the tag/file drawer
 */

#import "DrawerController.h"
#import "defs.h"
//#import "TagAPI.h"

// declare the private methods
@interface DrawerController (PrivateAPI)
- (void)setEnableAll:(BOOL)enable;
@end

@implementation DrawerController

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
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[displayedTrack release];
	[super dealloc];
}

/**********************/

- (void)awakeFromNib
{
	[drawerWindow setContentSize:[preferences drawerContentSize]];
	
	if ([preferences showDrawer])
	{
		[drawerWindow openOnEdge:[preferences drawerEdge]];
		[toggleButton setState:NSOnState];
	}
	else
	{
		[drawerWindow close];
		[toggleButton setState:NSOffState];
	}
	
	[self disableAll];
}

- (void)drawerDidClose:(NSNotification*)notification
{
  // change the toggle button to off
  [toggleButton setState:NSOffState];
	// don't save drawer edge so opens on sensible side
}

/* called when someone clicks on the tag button (bottom right)
 */
- (IBAction)toggleDrawer:(id)sender
{
	[drawerWindow toggle:sender];
}

/* called when someone clicks on the write tag button
 */
- (IBAction)writeTag:(id)sender
{
	if (writeTagTarget == nil || ![writeTagTarget respondsToSelector:writeTagSelector])
		return;
	
	Track *track = [self displayedTrackInfo];	
	
  [writeTagTarget performSelector:writeTagSelector withObject:track];
}

/* get the tag info displayed
 */
- (Track *)displayedTrackInfo
{
	// copy from initial track given
	Track *track = [[Track alloc] initWithTrack:displayedTrack];
	[track setTitle:[titleField stringValue]];
	[track setArtist:[artistField stringValue]];
	[track setAlbum:[albumField stringValue]];
	[track setGenre:[genreField stringValue]];
	[track setTrackNumber:(unsigned)[trackNumberField intValue]];
	[track setYear:(unsigned)[yearField intValue]];
	// other fields aren't changed
	[track autorelease];
	return track;
}

/* who should we tell when someone clicks on the write tag?
 * give them a Track * as an object
 */
- (void)setWriteTagSelector:(SEL)selector target:(id)target
{
	writeTagSelector = selector;
	writeTagTarget = target;
}

/* display the tag info in track
 */
- (void)showTrack:(Track *)track
{
	// don't do anything if we are told to show what we are already showing
	// this avoids removing someone's edits also
	if (displayedTrack == track)
		return;
	
	// empty all fields in case we don't fill them here
	// this will release displayedTrack but we know it's not the same as track so OK
	[self clearAll];
	
	[self enableAll];

	// do audio specific entries
	if ([track artist])
		[artistField setStringValue:[track artist]];
	if ([track album])
		[albumField setStringValue:[track album]];
	[trackNumberField setStringValue:[NSString stringWithFormat:@"%d", [track trackNumber]]];
	if ([track genre])
		[genreField setStringValue:[track genre]];
	[yearField setStringValue:[NSString stringWithFormat:@"%d", [track year]]];
	if ([track fileTypeString])
		[codecField setStringValue:[track fileTypeString]];
	if ([track lengthString])
		[lengthField setStringValue:[track lengthString]];
	
	if ([track title])
		[titleField setStringValue:[track title]];
	if ([track filesizeString])
		[filesizeField setStringValue:[track filesizeString]];
    
  if ([track image])
  {
    NSImage *image = [[NSImage alloc] init];
    [image addRepresentation:[track image]];
    [imageView setImage:image];
    [image release];
  }
  else
  {
    [imageView setImage:nil];
  }
	
	[track retain];
	[displayedTrack release];
	displayedTrack = track;
}

/* display the file info only in dataFile
 * disables other boxes (e.g. title, artist)
 */
- (void)showDataFile:(DataFile *)dataFile
{
	// disable and clear all, including write button
	[self disableAll];
	
	// might be the case if the item selected
	// has been deleted.
	if (dataFile == nil)
		return;
	
	[titleField setEnabled:YES];
	[titleField setEditable:NO];
	[filesizeField setEnabled:YES];
	[titleLabel setEnabled:YES];
	[filesizeLabel setEnabled:YES];
	
	if ([dataFile filename])
		[titleField setStringValue:[dataFile filename]];
	[filesizeField setStringValue:[dataFile sizeString]];
  
  [imageView setImage:nil];
	
	[displayedTrack release];
	displayedTrack = nil;
}

- (void)disableWrite
{
	[writeTagButton setEnabled:NO];
}

/* is the drawer open?
 */
- (BOOL)isOpen
{
	if ([drawerWindow state] == NSDrawerOpenState)
		return YES;
	else
		return NO;
}

- (void)applicationTerminating:(NSNotification *)note
{
	[preferences setShowDrawer:[self isOpen]];
	[preferences setDrawerContentSize:[drawerWindow contentSize]];
	[preferences setDrawerEdge:[drawerWindow edge]];
}

// enabling/disabling routines

- (void)disableAll
{
	[self setEnableAll:NO];
	[self clearAll];
}

- (void)enableAll
{
	[self setEnableAll:YES];
}

- (void)setEnableAll:(BOOL)enable
{
	[titleField setEnabled:enable];
	[artistField setEnabled:enable];
	[albumField setEnabled:enable];
	[trackNumberField setEnabled:enable];
	[genreField setEnabled:enable];
	[lengthField setEnabled:enable];
	[filesizeField setEnabled:enable];
	[codecField setEnabled:enable];
	[writeTagButton setEnabled:enable];
	[yearField setEnabled:enable];
	
	[titleField setEditable:enable];
	[artistField setEditable:enable];
	[albumField setEditable:enable];
	[trackNumberField setEditable:enable];
	[genreField setEditable:enable];
	[yearField setEditable:enable];
	
	[albumLabel setEnabled:enable];
	[artistLabel setEnabled:enable];
	[codecLabel setEnabled:enable];
	[filesizeLabel setEnabled:enable];
	[lengthLabel setEnabled:enable];
	[titleLabel setEnabled:enable];
	[trackNumberLabel setEnabled:enable];
	[genreLabel setEnabled:enable];
	[yearLabel setEnabled:enable];
}

- (void)clearAll
{
	[titleField setStringValue:@""];
	[artistField setStringValue:@""];
	[albumField setStringValue:@""];
	[trackNumberField setStringValue:@""];
	[genreField setStringValue:@""];
	[lengthField setStringValue:@""];
	[filesizeField setStringValue:@""];
	[codecField setStringValue:@""];
	[yearField setStringValue:@""];
  [imageView setImage:nil];
	[displayedTrack release];
	displayedTrack = nil;
}

@end
