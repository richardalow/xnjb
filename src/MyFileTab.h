//
//  MyFileTab.h
//  XNJB
//

#import <Cocoa/Cocoa.h>
#import "MyTableTab.h"
#import "FileSystemBrowser.h"
#import "MyItem.h"

typedef enum _activeObjects {OBJECT_NJB_TABLE = 0, OBJECT_FILE_BROWSER = 1, OBJECT_OTHER = 2} activeObjects;

@interface MyFileTab : MyTableTab
{
	IBOutlet NSBrowser *fileBrowser;
	IBOutlet NSButton *copyToNJBButton;
	IBOutlet NSButton *copyFromNJBButton;
	IBOutlet NSButton *deleteFromActiveObjectButton;
	FileSystemBrowser *browser;
	activeObjects activeObject;
}
- (IBAction)copyFromNJB:(id)sender;
- (IBAction)copyToNJB:(id)sender;
- (IBAction)deleteFromActiveObject:(id)sender;

- (void)drawerWriteTag:(Track *)modifiedTrack;
- (void)onFirstResponderChange:(NSResponder *)aResponder;
- (void)menuDelete;
- (BOOL)menuShouldDelete;
- (void)setActiveObject:(activeObjects)newActiveObject;
- (activeObjects)activeObject;
- (void)NJBConnected:(NSNotification *)note;
- (void)NJBDisconnected:(NSNotification *)note;
- (BOOL)menuShouldDelete;
- (void)deleteFromNJB;
- (void)deleteFromFileSystem;
- (void)copyFileToNJB:(NSString *)filename;
- (void)copyFileFromNJB:(MyItem *)item;
- (void)deleteFromFileSystem;
- (void)deleteFromNJB:(MyItem *)item;
- (void)disableTransfers;
- (void)enableTransfers;
- (BOOL)canCopyToNJB;
- (BOOL)canCopyFromNJB;
- (void)fileSystemBrowserFileClick:(NSString *)path;
- (void)fileSystemBrowserDirectoryClick:(NSString *)path;
- (NSString *)browserDirectory;
- (void)applicationTerminating:(NSNotification *)note;
- (NSString *)replaceBadFilenameChars:(NSString *)filename;
- (BOOL)canDelete;
- (void)addToArray:(MyItem *)item;
- (void)removeFromArray:(MyItem *)item;
- (void)showDrawerInfo;
- (NSString *)browserPathFromPreferences;

@end
