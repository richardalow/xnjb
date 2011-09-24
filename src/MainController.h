//
//  MainController.h
//  XNJB
//

#import <Cocoa/Cocoa.h>
#import "DataTab.h"
#import "MusicTab.h"
#import "QueueTab.h"
#import "PlaylistsTab.h"
#import "DrawerController.h"
#import "MyNSWindow.h"
#import "NJBQueueConsumer.h"
#import "Queue.h"
#import "NJB.h"
#import "StatusDisplayer.h"
#import "SettingsTab.h"
#import "Preferences.h"
#import "DuplicateItemsTab.h"
#import "InfoTab.h"

@interface MainController : NSObject
{
@private
	IBOutlet DataTab *dataTab;
	IBOutlet MusicTab *musicTab;
	IBOutlet PlaylistsTab *playlistsTab;
	IBOutlet QueueTab *queueTab;
	IBOutlet SettingsTab *settingsTab;
	IBOutlet DuplicateItemsTab *duplicateItemsTab;
	IBOutlet InfoTab *infoTab;
	IBOutlet DrawerController *drawerController;
	IBOutlet NSProgressIndicator *progressBar;
	IBOutlet NSTabView *tabViewMain;
	IBOutlet MyNSWindow *windowMain;
	IBOutlet NSTextField *statusText;
	IBOutlet NSTextField *diskSpaceText;
  IBOutlet Preferences *preferences;
	IBOutlet NJB *theNJB;
	IBOutlet StatusDisplayer *status;
	IBOutlet NSButton *connectButton;
	NJBQueueConsumer *njbQueue;
	Queue *myQueue;
	NSLock *myQueueLock;
	NSConnection *kitConnection;
	unsigned previousTabIndex;
}

- (IBAction)buttonConnect:(id)sender;
- (void)drawerWriteTag:(Track *)modifiedTrack;
- (void)initNJB;
- (id)currentTab;
- (int)currentTabIndex;
- (id)tabAtIndex:(unsigned)index;
- (void)postNotificationName:(NSString *)name object:(id)object;
- (void)connectToNJB;
- (void)disconnectFromNJB;
- (BOOL)shouldClose;

- (DataTab *)dataTab;
- (MusicTab *)musicTab;
- (PlaylistsTab *)playlistsTab;
- (QueueTab *)queueTab;
- (SettingsTab *)settingsTab;
- (DuplicateItemsTab *)duplicateItemsTab;
- (InfoTab *)infoTab;

- (BOOL)menuShouldConnect;
- (BOOL)menuShouldDisconnect;
- (void)menuConnect;
- (void)menuDisconnect;

@end
