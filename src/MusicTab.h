//
//  MusicTab.h
//  XNJB
//

#import <Cocoa/Cocoa.h>
#import "MyFileTab.h"
#import "NJBTransactionResult.h"
#import "MyTableTab.h"
#import "MyNSArrayController.h"

@interface MusicTab : MyFileTab
{
@private
	IBOutlet MyTableTab *playlistsTab;
	activeObjects objectToSaveTrack;
	BOOL trackListUpToDate;
	IBOutlet MyNSArrayController *iTunesArrayController;
	IBOutlet NSTextField *iTunesSearchField;
	IBOutlet NSTextField *iTunesItemCountField;
  IBOutlet NSTableView *iTunesTable;
	IBOutlet NSBox *iTunesBox;
  IBOutlet NSWindow *windowMain;
}
- (void)loadTracks;
- (NJBTransactionResult *)downloadTrackList;
- (void)copyTrackToNJB:(Track *)track;
- (void)copyTrackFromNJB:(Track *)track;
- (void)replaceTrack:(Track *)replace withTrack:(Track *)new;
- (void)writeTagToFile:(Track *)track;
- (NJBTransactionResult *)uploadTrack:(Track *)track;
- (void)addTrackToiTunes:(Track *)track;
- (IBAction)searchiTunesTableAction:(id)sender;
- (void)setCorrectView;

@end
