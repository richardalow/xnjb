//
//  DrawerController.h
//  XNJB
//

#import <Cocoa/Cocoa.h>
#import "Track.h"
#import "DataFile.h"
#import "Preferences.h"

@interface DrawerController : NSObject
{
@private
	IBOutlet NSTextField *albumField;
  IBOutlet NSTextField *artistField;
  IBOutlet NSTextField *codecField;
  IBOutlet NSDrawer *drawerWindow;
  IBOutlet NSTextField *filesizeField;
  IBOutlet NSTextField *genreField;
  IBOutlet NSTextField *lengthField;
	IBOutlet NSTextField *titleField;
	IBOutlet NSButton *toggleButton;
	IBOutlet NSTextField *trackNumberField;
	IBOutlet NSButton *writeTagButton;
	IBOutlet NSTextField *yearField;
	IBOutlet Preferences *preferences;
	IBOutlet NSTextField *albumLabel;
	IBOutlet NSTextField *artistLabel;
	IBOutlet NSTextField *codecLabel;
	IBOutlet NSTextField *filesizeLabel;
	IBOutlet NSTextField *lengthLabel;
	IBOutlet NSTextField *titleLabel;
	IBOutlet NSTextField *trackNumberLabel;
	IBOutlet NSTextField *genreLabel;
	IBOutlet NSTextField *yearLabel;
  IBOutlet NSImageView *imageView;
	
	SEL writeTagSelector;
	id writeTagTarget;
	Track *displayedTrack;
}
- (IBAction)toggleDrawer:(id)sender;

- (void)showTrack:(Track *)track;
- (void)disableAll;
- (void)enableAll;
- (void)clearAll;
- (void)setWriteTagSelector:(SEL)selector target:(id)target;
- (Track *)displayedTrackInfo;
- (IBAction)writeTag:(id)sender;
- (void)disableWrite;
- (void)showDataFile:(DataFile *)dataFile;
- (BOOL)isOpen;
- (void)applicationTerminating:(NSNotification *)note;

@end
