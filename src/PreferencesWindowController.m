//
//  PreferencesWindowController.m
//  XNJB
//
//  Created by Richard Low on Tue Aug 31 2004.
//

/* Controls the preferences window. Read in preferences
 * and displays in text boxes, check boxes, etc. Then writes
 * back to preferences object.
 */

#import "defs.h"
#import "PreferencesWindowController.h"

// declare the private methods
@interface PreferencesWindowController (PrivateAPI)
- (void)displayPreferences;
- (BOOL)isDirty;
@end

@implementation PreferencesWindowController

// init/dealloc methods

- (id)init
{
	self = [super initWithWindowNibName:@"Preferences"];
	return self;
}

- (void)dealloc
{
	[preferences release];
	[tabView release];
	[super dealloc];
}

// accessor methods

- (void)setPreferences:(Preferences *)newPreferences
{
	[newPreferences retain];
	[preferences release];
	preferences = newPreferences;
}

- (void)setTabView:(NSTabView *)newTabView
{
	[newTabView retain];
	[tabView release];
	tabView = newTabView;
}

/**********************/

- (void)awakeFromNib
{
	[[self window] setFrameAutosaveName:@"preferencesWindowFrame"];
	
	[self displayPreferences];
}

/* Show the current preferences
 */
- (void)displayPreferences
{
	[self setStateForButton:connectOnStartup fromBool:[preferences connectOnStartup]];
	[self setStateForButton:writeTagAfterCopy fromBool:[preferences writeTagAfterCopy]];
	[self setStateForButton:enableTurbo fromBool:[preferences turbo]];
	[self setStateForButton:startupTabLastUsed fromBool:[preferences startupTabLastUsed]];
	if (![preferences startupTabLastUsed])
	{
		[startupTab selectItemAtIndex:[preferences startupTab]];
		[startupTab setEnabled:YES];
	}
	else
	{
		[startupTab setEnabled:NO];
	}
	[self setStateForButton:musicTabDirLastUsed fromBool:[preferences musicTabDirLastUsed]];
	if (![preferences musicTabDirLastUsed])
	{
		[musicTabDir setStringValue:[preferences musicTabDir]];
		[changeMusicTabDir setEnabled:YES];
		[musicTabDir setEnabled:YES];
	}
	else
	{
		[changeMusicTabDir setEnabled:NO];
		[musicTabDir setEnabled:NO];
	}
	[self setStateForButton:dataTabDirLastUsed fromBool:[preferences dataTabDirLastUsed]];
	if (![preferences dataTabDirLastUsed])
	{
		[dataTabDir setStringValue:[preferences dataTabDir]];
		[changeDataTabDir setEnabled:YES];
		[dataTabDir setEnabled:YES];
	}
	else
	{
		[changeDataTabDir setEnabled:NO];
		[dataTabDir setEnabled:NO];
	}
	[filenameFormat setStringValue:[preferences filenameFormat]];
	
	[duplicatesLengthTol setIntValue:[preferences lengthTol]];
	[duplicatesLengthTolStepper setIntValue:[preferences lengthTol]];
	[duplicatesFilesizeTol setIntValue:[preferences filesizeTol]];
	[duplicatesFilesizeTolStepper setIntValue:[preferences filesizeTol]];
	[duplicatesTitleTol setIntValue:[preferences titleTol]];
	[duplicatesTitleTolStepper setIntValue:[preferences titleTol]];
	[duplicatesArtistTol setIntValue:[preferences artistTol]];
	[duplicatesArtistTolStepper setIntValue:[preferences artistTol]];
	
	[self setStateForButton:iTunesIntegration fromBool:[preferences iTunesIntegration]];
	if (![preferences iTunesIntegration])
	{
		[downloadedTrackDir setEnabled:NO];
		[iTunesDirectory setEnabled:NO];
	}
	else
	{
		[downloadedTrackDir setEnabled:YES];
		[iTunesDirectory setEnabled:YES];
	}
	// show these anyway
	[downloadedTrackDir setStringValue:[preferences downloadDir]];
	[iTunesDirectory setStringValue:[preferences iTunesDirectory]];
  
  // so we get defaults
  [albumArtWidth setIntValue:[preferences albumArtWidth]];
  [albumArtHeight setIntValue:[preferences albumArtHeight]];
    
  [self setStateForButton:createAlbumFiles fromBool:[preferences createAlbumFiles]];
  if ([preferences createAlbumFiles])
  {
    [uploadAlbumArt setEnabled:YES];
    [self setStateForButton:uploadAlbumArt fromBool:[preferences uploadAlbumArt]];
    
    if ([preferences uploadAlbumArt])
    {
      [albumArtWidth setEnabled:YES];
      [albumArtHeight setEnabled:YES];
    }
    else
    {
      [albumArtWidth setEnabled:NO];
      [albumArtHeight setEnabled:NO];
    }
  }
  else
  {
    [uploadAlbumArt setEnabled:NO];
    [albumArtWidth setEnabled:NO];
    [albumArtHeight setEnabled:NO];
  }
	
	// start with no changes
	// this goes at the end as setting text above causes boxes to call controlTextDidChange
	isDirty = NO;
}

/* Sets the button state mapping YES->NSOnState, NO->NSOffState
 */
- (void)setStateForButton:(NSButton *)button fromBool:(BOOL)state
{
	if (state)
		[button setState:NSOnState];
	else
		[button setState:NSOffState];
}

/* reverse of setStateForButton:
 */
- (BOOL)boolForButton:(NSButton *)button
{
	if ([button state] == NSOnState)
		return YES;
	else
		return NO;
}

/* Show the window
 */
- (void)showWindow:(id)sender
{
	// reload data for combo box as tabView is nil when we get called on startup
	[startupTab reloadData];
	[self displayPreferences];
	[super showWindow:sender];
}

/* Save the preferences
 */
- (IBAction)buttonOK:(id)sender
{
	[preferences setConnectOnStartup:[self boolForButton:connectOnStartup]];
	[preferences setWriteTagAfterCopy:[self boolForButton:writeTagAfterCopy]];
	[preferences setTurbo:[self boolForButton:enableTurbo]];
	if ([startupTabLastUsed state] == NSOnState)
		[preferences setStartupTabLastUsed:YES];
	else
	{
		[preferences setStartupTabLastUsed:NO];
		[preferences setStartupTab:[startupTab indexOfSelectedItem]];
	}
	if ([musicTabDirLastUsed state] == NSOnState)
		[preferences setMusicTabDirLastUsed:YES];
	else
	{
		[preferences setMusicTabDirLastUsed:NO];
		[preferences setMusicTabDir:[musicTabDir stringValue]];
	}
	if ([dataTabDirLastUsed state] == NSOnState)
		[preferences setDataTabDirLastUsed:YES];
	else
	{
		[preferences setDataTabDirLastUsed:NO];
		[preferences setDataTabDir:[dataTabDir stringValue]];
	}
	[preferences setFilenameFormat:[filenameFormat stringValue]];
	
	[preferences setLengthTol:[duplicatesLengthTol intValue]];
	[preferences setFilesizeTol:[duplicatesFilesizeTol intValue]];
	[preferences setTitleTol:[duplicatesTitleTol intValue]];
	[preferences setArtistTol:[duplicatesArtistTol intValue]];
	
	if ([iTunesIntegration state] == NSOnState)
	{
		[preferences setiTunesIntegration:YES];
		[preferences setDownloadDir:[downloadedTrackDir stringValue]];
		[preferences setiTunesDirectory:[iTunesDirectory stringValue]];
	}
	else
	{
		[preferences setiTunesIntegration:NO];
	}
  
  [preferences setCreateAlbumFiles:[self boolForButton:createAlbumFiles]];
  if ([preferences createAlbumFiles])
  {
    [preferences setUploadAlbumArt:[self boolForButton:uploadAlbumArt]];
    if ([preferences uploadAlbumArt])
    {
      [preferences setAlbumArtWidth:[albumArtWidth intValue]];
      [preferences setAlbumArtHeight:[albumArtHeight intValue]];
    }
  }
	
	[self close];
}

- (IBAction)buttonCancel:(id)sender
{
	[self close];
}

/* Show open panel to choose a new Music tab directory
 */
- (IBAction)changeMusicTabDir:(id)sender
{
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	
	[panel setCanChooseFiles:NO];
	[panel setCanChooseDirectories:YES];
	
	[panel beginSheetForDirectory:[musicTabDir stringValue]
													 file:nil
													types:nil
								 modalForWindow:[self window]
									modalDelegate:self
								 didEndSelector:@selector(changeMusicTabDirPanelDidEnd:returnCode:contextInfo:)
										contextInfo:NULL];
}

- (void)changeMusicTabDirPanelDidEnd:(NSOpenPanel *)openPanel returnCode:(int)returnCode contextInfo:(void *)x
{
	if (returnCode == NSOKButton)
	{
		[musicTabDir setStringValue:[openPanel filename]];
		isDirty = YES;
	}
}

/* Show open panel to choose a new Data tab directory
*/
- (IBAction)changeDataTabDir:(id)sender
{
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	
	[panel setCanChooseFiles:NO];
	[panel setCanChooseDirectories:YES];
	
	[panel beginSheetForDirectory:[dataTabDir stringValue]
													 file:nil
													types:nil
								 modalForWindow:[self window]
									modalDelegate:self
								 didEndSelector:@selector(changeDataTabDirPanelDidEnd:returnCode:contextInfo:)
										contextInfo:NULL];
}

- (void)changeDataTabDirPanelDidEnd:(NSOpenPanel *)openPanel returnCode:(int)returnCode contextInfo:(void *)x
{
	if (returnCode == NSOKButton)
	{
		[dataTabDir setStringValue:[openPanel filename]];
		isDirty = YES;
	}
}

- (IBAction)startupTabLastUsed:(id)sender
{
	if ([startupTabLastUsed state] == NSOnState)
		[startupTab setEnabled:NO];
	else
	{
		[startupTab setEnabled:YES];
		// select the current tab if none was selected before
		if ([startupTab indexOfSelectedItem] == -1)
			[startupTab selectItemAtIndex:[tabView indexOfTabViewItem:[tabView selectedTabViewItem]]];
	}
	isDirty = YES;
}

- (IBAction)musicTabDirLastUsed:(id)sender
{
	if ([musicTabDirLastUsed state] == NSOnState)
	{
		[musicTabDir setEnabled:NO];
		[changeMusicTabDir setEnabled:NO];
	}
	else
	{
		[musicTabDir setEnabled:YES];
		[changeMusicTabDir setEnabled:YES];
	}
	isDirty = YES;
}

- (IBAction)dataTabDirLastUsed:(id)sender
{
	if ([dataTabDirLastUsed state] == NSOnState)
	{	
		[dataTabDir setEnabled:NO];
		[changeDataTabDir setEnabled:NO];
	}
	else
	{
		[dataTabDir setEnabled:YES];
		[changeDataTabDir setEnabled:YES];
	}
	isDirty = YES;
}

- (int)numberOfItemsInComboBox:(NSComboBox *)aComboBox
{
	return [tabView numberOfTabViewItems];
}

- (id)comboBox:(NSComboBox *)aComboBox objectValueForItemAtIndex:(int)index
{
	return [[tabView tabViewItemAtIndex:index] label];
}

- (BOOL)windowShouldClose:(id)sender
{
	if ([self isDirty])
	{
		int result = NSRunAlertPanel(NSLocalizedString(@"Save Preferences", nil),
																 NSLocalizedString(@"Do you want to save changes to preferences?", nil),
																 NSLocalizedString(@"Yes", nil),
																 NSLocalizedString(@"No", nil),
																 NSLocalizedString(@"Cancel", nil));
		if (result == NSAlertDefaultReturn)
		{
			// Yes
			[self buttonOK:sender];
			return YES;
		}
		else if (result == NSAlertAlternateReturn)
		{
			// No
			return YES;
		}
		else
		{
			// cancel
			return NO;
		}
	}
	else
		return YES;
}

- (void)controlTextDidChange:(NSNotification *)aNotification
{
	isDirty = YES;
}

- (IBAction)connectOnStartup:(id)sender
{
	isDirty = YES;
}

- (IBAction)writeTagAfterCopy:(id)sender
{
	isDirty = YES;
}

- (void)comboBoxSelectionDidChange:(NSNotification *)notification
{
	isDirty = YES;
}

- (IBAction)enableTurbo:(id)sender
{
	isDirty = YES;
}

- (IBAction)createAlbumFiles:(id)sender
{
	isDirty = YES;
  if ([createAlbumFiles state] == NSOnState)
  {
    [uploadAlbumArt setEnabled:YES];
    [self uploadAlbumArt:sender];
  }
  else
  {
    [uploadAlbumArt setEnabled:NO];
    [albumArtHeight setEnabled:NO];
    [albumArtWidth setEnabled:NO];
  }
}

- (IBAction)uploadAlbumArt:(id)sender
{
	isDirty = YES;
  if ([uploadAlbumArt state] == NSOnState)
  {
    [albumArtHeight setEnabled:YES];
    [albumArtWidth setEnabled:YES];
  }
  else
  {
    [albumArtHeight setEnabled:NO];
    [albumArtWidth setEnabled:NO];
  }
}

- (BOOL)isDirty
{
	if (isDirty)
		return YES;
	// check to see if any have changed: easier than finding out when stepper clicked
	if ([preferences lengthTol] != [duplicatesLengthTol intValue] ||
			[preferences filesizeTol] != [duplicatesFilesizeTol intValue] ||
			[preferences titleTol] != [duplicatesTitleTol intValue] ||
			[preferences artistTol] != [duplicatesArtistTol intValue])
		return YES;
	return NO;
}

- (IBAction)iTunesIntegration:(id)sender
{
	if ([iTunesIntegration state] == NSOnState)
	{	
		[downloadedTrackDir setEnabled:YES];
		[iTunesDirectory setEnabled:YES];
	}
	else
	{
		[downloadedTrackDir setEnabled:NO];
		[iTunesDirectory setEnabled:NO];
	}
	isDirty = YES;
}

@end
