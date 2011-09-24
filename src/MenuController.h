//
//  MenuController.h
//  XNJB
//

#import <Cocoa/Cocoa.h>
#import "MainController.h"
#import "MyTab.h"
#import "MyFileTab.h"
#import "PreferencesWindowController.h"
#import "Preferences.h"

@interface MenuController : NSObject
{
	IBOutlet MainController *mainController;
	IBOutlet Preferences *preferences;
	IBOutlet NSTabView *tabView;
	PreferencesWindowController *preferencesController;
}
- (IBAction)copyToComputer:(id)sender;
- (IBAction)copyToNJB:(id)sender;
- (IBAction)delete:(id)sender;
- (IBAction)connect:(id)sender;
- (IBAction)disconnect:(id)sender;
- (IBAction)cancelQueuedItem:(id)sender;
- (IBAction)preferences:(id)sender;
@end
