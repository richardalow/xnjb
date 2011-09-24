//
//  PreferencesWindowController.h
//  XNJB
//
//  Created by Richard Low on Tue Aug 31 2004.
//

#import <Foundation/Foundation.h>
#import "Preferences.h"

@interface PreferencesWindowController : NSWindowController {
  IBOutlet NSButton *connectOnStartup;
	IBOutlet NSComboBox *startupTab;
	IBOutlet NSButton *startupTabLastUsed;
	IBOutlet NSTextField *musicTabDir;
	IBOutlet NSButton *musicTabDirLastUsed;
	IBOutlet NSTextField *dataTabDir;
	IBOutlet NSButton *dataTabDirLastUsed;
	IBOutlet NSTextField *filenameFormat;
	IBOutlet NSButton *changeMusicTabDir;
	IBOutlet NSButton *changeDataTabDir;
	IBOutlet NSButton *writeTagAfterCopy;
	IBOutlet NSTextField *duplicatesLengthTol;
	IBOutlet NSTextField *duplicatesFilesizeTol;
	IBOutlet NSTextField *duplicatesTitleTol;
	IBOutlet NSTextField *duplicatesArtistTol;
	IBOutlet NSStepper *duplicatesLengthTolStepper;
	IBOutlet NSStepper *duplicatesFilesizeTolStepper;
	IBOutlet NSStepper *duplicatesTitleTolStepper;
	IBOutlet NSStepper *duplicatesArtistTolStepper;
	IBOutlet NSButton *enableTurbo;
	IBOutlet NSButton *iTunesIntegration;
	IBOutlet NSTextField *downloadedTrackDir;
	IBOutlet NSTextField *iTunesDirectory;
  IBOutlet NSButton *createAlbumFiles;
  IBOutlet NSButton *uploadAlbumArt;
  IBOutlet NSTextField *albumArtHeight;
  IBOutlet NSTextField *albumArtWidth;
	
	Preferences *preferences;
	NSTabView *tabView;
	BOOL isDirty;
}
- (void)setStateForButton:(NSButton *)button fromBool:(BOOL)state;
- (void)changeMusicTabDirPanelDidEnd:(NSOpenPanel *)openPanel returnCode:(int)returnCode contextInfo:(void *)x;
- (void)changeDataTabDirPanelDidEnd:(NSOpenPanel *)openPanel returnCode:(int)returnCode contextInfo:(void *)x;
- (BOOL)boolForButton:(NSButton *)button;
- (void)setPreferences:(Preferences *)newPreferences;
- (void)setTabView:(NSTabView *)newTabView;

- (IBAction)buttonOK:(id)sender;
- (IBAction)buttonCancel:(id)sender;
- (IBAction)connectOnStartup:(id)sender;
- (IBAction)changeMusicTabDir:(id)sender;
- (IBAction)changeDataTabDir:(id)sender;
- (IBAction)startupTabLastUsed:(id)sender;
- (IBAction)musicTabDirLastUsed:(id)sender;
- (IBAction)dataTabDirLastUsed:(id)sender;
- (IBAction)writeTagAfterCopy:(id)sender;
- (IBAction)enableTurbo:(id)sender;
- (IBAction)iTunesIntegration:(id)sender;
- (IBAction)createAlbumFiles:(id)sender;
- (IBAction)uploadAlbumArt:(id)sender;

@end
