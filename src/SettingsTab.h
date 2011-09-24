//
//  SettingsTab.h
//  XNJB
//

#import <Cocoa/Cocoa.h>
#import "MyTab.h"
#import "NJBTransactionResult.h"

@interface SettingsTab : MyTab
{
  IBOutlet NSTextField *ownerText;
  IBOutlet NSButton *setToComputerTimeButton;
  IBOutlet NSTextField *timeText;
	//IBOutlet NSButton *setBitmapButton;
	IBOutlet NSButton *updateJukeboxButton;
	IBOutlet NSWindow *mainWindow;
	//IBOutlet NSTextField *bitmapText;
	IBOutlet NSTextField *ownerLabel;
	IBOutlet NSTextField *timeLabel;
	//IBOutlet MyNSTextField *setBitmapLabel;
	NSString *ownerString;
  NSTimer *timer;
	/*
	 we store the jukebox time as a time interval since now so that if we miss an update we don't get behind
	 and when we send to update if it doesn't update immediately it is still correct
		*/
	NSTimeInterval jukeboxTimeInterval;
	BOOL showTime;
	//NSString *bitmapPath;
}
- (IBAction)setToComputerTime:(id)sender;
- (IBAction)updateJukebox:(id)sender;
//- (IBAction)setBitmap:(id)sender;
- (void)loadSettings;
- (NJBTransactionResult *)downloadSettings;
- (NJBTransactionResult *)updateDisplay;
- (void)setOwnerString:(NSString *)newOwnerString;
- (void)setJukeboxTime:(NSCalendarDate *)newJukeboxTime;
- (void)updateJukeboxTime:(NSTimer *)aTimer;
- (NSCalendarDate *)jukeboxTime;
- (void)startTimer;
- (void)stopTimer;
//- (void)openPanelDidEnd:(NSOpenPanel *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo;
@end
