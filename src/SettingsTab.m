//
//  SettingsTab.m
//  XNJB
//

/* Class for the settings tab.
 * Implemented so far is owner string and time/data
 * Doesn't support bitmap because I couldn't find enough info
 * on what size/depth for bitmaps.
 * The time is stored as a difference between the jukebox time
 * and the computer time. If the user updates the computer clock
 * then this will affect the time. Good in some cases: when time
 * changes for daylight saving. Could be confusing though.
 */

#import "defs.h"
#import "SettingsTab.h"

@implementation SettingsTab

// init/dealloc methods

- (void)dealloc
{
	[ownerString release];
	[timer release];
	//[bitmapPath release];
	[super dealloc];
}

/**********************/

- (void)awakeFromNib
{
	[super awakeFromNib];
	[self startTimer];
}

/* Set the jukebox time to the computer time
 */
- (IBAction)setToComputerTime:(id)sender
{
	[self setJukeboxTime:[NSCalendarDate calendarDate]];
	[timeText setObjectValue:[self jukeboxTime]];
}

/* Start the timer to update the jukebox time
 */
- (void)startTimer
{
	if (timer != nil)
	{
		// likely to get here as double clicking on the text calls stop editing but no start
		NSLog(@"timer already started!");
		return;
	}
	timer = [[NSTimer scheduledTimerWithTimeInterval:1.0 target:self selector:@selector(updateJukeboxTime:) userInfo:nil repeats:YES] retain];
}

/* stop the timer (called when user starts editing the time)
 */
- (void)stopTimer
{
	[timer invalidate];
	[timer release];
	// so we can't release again
	timer = nil;
}

/* update the time in the box - the target of the timer so called
 * every second
 */
- (void)updateJukeboxTime:(NSTimer *)aTimer
{
	if (showTime)
		[timeText setObjectValue:[self jukeboxTime]];
}

- (void)onConnectAndActive
{
	[super onConnectAndActive];
	[self loadSettings];
}

- (void)loadSettings
{
	NJBQueueItem *getSettings = [[NJBQueueItem alloc] initWithTarget:self withSelector:@selector(downloadSettings)];
	[getSettings setStatus:STATUS_DOWNLOADING_SETTINGS];
	
	NJBQueueItem *updateDisplay = [[NJBQueueItem alloc] initWithTarget:self
																											withSelector:@selector(updateDisplay)
																												withObject:nil
																							 withRunInMainThread:YES];
	[updateDisplay setDisplayStatus:NO];
	
	NSString *description = NSLocalizedString(@"Getting Jukebox settings", nil);
	[self addToQueue:getSettings withSubItems:[NSArray arrayWithObjects:updateDisplay, nil] withDescription:description];
	[getSettings release];
	[updateDisplay release];
}

/* this method downloads the settings from the NJB.
 * Designed to be run in the worker thread, so does not update the text field
 */
- (NJBTransactionResult *)downloadSettings
{
	[self setOwnerString:[theNJB ownerString]];
	if (ownerString == nil)
		return [[[NJBTransactionResult alloc] initWithSuccess:NO] autorelease];
	if (![theNJB isMTPDevice])
	{
		NSCalendarDate *date = [theNJB jukeboxTime];
		if (date == nil)
			return [[[NJBTransactionResult alloc] initWithSuccess:NO] autorelease];
		[self setJukeboxTime:date];
		showTime = YES;
	}
	return [[[NJBTransactionResult alloc] initWithSuccess:YES] autorelease];
}

/* this method actually updates the text boxes,
 * so must be run in the main thread 
 */
- (NJBTransactionResult *)updateDisplay
{
	[ownerText setStringValue:ownerString];
	if (showTime)
		[timeText setObjectValue:[self jukeboxTime]];
	return [[[NJBTransactionResult alloc] initWithSuccess:YES] autorelease];
}

- (void)setOwnerString:(NSString *)newOwnerString
{
	[newOwnerString retain];
	[ownerString release];
	ownerString = newOwnerString;
}

- (void)setJukeboxTime:(NSCalendarDate *)newJukeboxTime
{
	jukeboxTimeInterval = [newJukeboxTime timeIntervalSinceNow];
}

- (void)controlTextDidBeginEditing:(NSNotification *)aNotification
{
	[self stopTimer];
}

- (void)controlTextDidEndEditing:(NSNotification *)aNotification
{
	// time changed
	[self setJukeboxTime:[timeText objectValue]];
	[self startTimer];
}

- (NSCalendarDate *)jukeboxTime
{
	return [NSCalendarDate dateWithTimeIntervalSinceNow:jukeboxTimeInterval];
}

- (void)NJBDisconnected:(NSNotification *)note
{
	showTime = NO;
	//[setBitmapButton setEnabled:NO];
	[updateJukeboxButton setEnabled:NO];
	[timeText setEnabled:NO];
	[setToComputerTimeButton setEnabled:NO];
	[ownerText setEnabled:NO];
	//[bitmapText setEnabled:NO];
	//[setBitmapLabel setEnabled:NO];
	[ownerLabel setEnabled:NO];
	[timeLabel setEnabled:NO];

	[super NJBDisconnected:note];
}

- (void)NJBConnected:(NSNotification *)note
{	
	//if ([theNJB isProtocol3Device])
		//[setBitmapButton setEnabled:YES];
	[updateJukeboxButton setEnabled:YES];
	if (![theNJB isMTPDevice])
	{
		[timeText setEnabled:YES];
		[setToComputerTimeButton setEnabled:YES];
		[timeLabel setEnabled:YES];
	}
	else
	{
		[timeText setEnabled:NO];
		[setToComputerTimeButton setEnabled:NO];
		[timeLabel setEnabled:NO];
	}
	[ownerText setEnabled:YES];
	//[bitmapText setEnabled:YES];
	//[bitmapText setStringValue:@""];
	//[setBitmapLabel setEnabled:YES];
	[ownerLabel setEnabled:YES];
	
	[super NJBConnected:note];
}

/* Actually update the jukebox, called when user clicks
 * on update button
 */
- (IBAction)updateJukebox:(id)sender
{
	NSString *owner = [ownerText stringValue];
	NSCalendarDate *time = [timeText objectValue];
	
	NJBQueueItem *setOwner = [[NJBQueueItem alloc] initWithTarget:theNJB withSelector:@selector(setOwnerString:)
																												withObject:owner];
	[setOwner setStatus:STATUS_UPDATING_SETTINGS];
	
	[self addToQueue:setOwner withSubItems:nil withDescription:[NSString stringWithFormat:NSLocalizedString(@"Setting Owner Name To %@", nil), owner]];
	[setOwner release];
	
	if (![theNJB isMTPDevice])
	{
		NJBQueueItem *setTime = [[NJBQueueItem alloc] initWithTarget:theNJB withSelector:@selector(setTime:)
																										withObject:[NSNumber numberWithDouble:jukeboxTimeInterval]];
		[setTime setStatus:STATUS_UPDATING_SETTINGS];
	
		[self addToQueue:setTime withSubItems:nil withDescription:[NSString stringWithFormat:NSLocalizedString(@"Setting Time To %@", nil), time]];
		[setTime release];
	}
	
	/*if (bitmapPath != nil)
	{
		NJBQueueItem *setBitmap = [[NJBQueueItem alloc] initWithTarget:theNJB withSelector:@selector(setBitmap:)
																											withObject:bitmapPath];
		[setBitmap setStatus:STATUS_UPDATING_SETTINGS];
		
		[self addToQueue:setBitmap withSubItems:nil withDescription:[NSString stringWithFormat:@"Setting Bitmap To %@", bitmapPath]];
		
		[setBitmap release];
	}*/
}
/*
- (IBAction)setBitmap:(id)sender
{
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	
	[panel beginSheetForDirectory:nil
													 file:nil
													types:[NSArray arrayWithObject:@"bmp"]
								 modalForWindow:mainWindow
									modalDelegate:self
								 didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:)
										contextInfo:NULL];
}

- (void)openPanelDidEnd:(NSOpenPanel *)openPanel returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
	if (returnCode == NSOKButton)
	{
		[bitmapPath release];
		bitmapPath = [[openPanel filename] retain];
		// todo: if too long, add ... as in browser
		[bitmapText setStringValue:bitmapPath];
		NSLog(@"bitmapPath: %@", bitmapPath);
	}
}
*/
@end
