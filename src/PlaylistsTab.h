//
//  PlaylistsTab.h
//  XNJB
//

#import <Cocoa/Cocoa.h>
#import "MyTableTab.h"

@interface PlaylistsTab : MyTableTab
{
@private
	IBOutlet MyTableTab *musicTab;
	IBOutlet NSTableView *playlistsTable;
	IBOutlet NSTableView *playlistsTrackTable;
	IBOutlet NSButton *addToPlaylistButton;
	IBOutlet NSButton *deleteFromPlaylistButton;
	IBOutlet NSButton *newPlaylistButton;
	IBOutlet NSButton *deletePlaylistButton;
	IBOutlet NSButton *moveTracksUpButton;
	IBOutlet NSButton *moveTracksDownButton;
	NSMutableArray *playlists;
	NSMutableArray *tracksInCurrentPlaylist;
	BOOL trackListUpToDate;
}
- (IBAction)addToPlaylist:(id)sender;
- (IBAction)deleteFromPlaylist:(id)sender;
- (IBAction)newPlaylist:(id)sender;
- (IBAction)deletePlaylist:(id)sender;
- (IBAction)moveTracksUp:(id)sender;
- (IBAction)moveTracksDown:(id)sender;
- (void)loadTracks;
- (NJBTransactionResult *)downloadTrackList;
- (void)loadPlaylists;
- (NJBTransactionResult *)downloadPlaylists;
- (void)updateJukeboxPlaylist:(Playlist *)playlist;
- (void)deleteJukeboxPlaylist:(Playlist *)playlist;

@end
