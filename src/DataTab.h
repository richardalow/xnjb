//
//  DataTab.h
//  XNJB
//

#import <Cocoa/Cocoa.h>
#import "MyFileTab.h"
#import "NJBTransactionResult.h"
#import "Directory.h"
#import "MyNSBrowser.h"

@interface DataTab : MyFileTab
{
	// we should never modify this - let NJB do it and update from there
	Directory *baseDir;
	Directory *tempBaseDir;
	IBOutlet MyNSBrowser *njbBrowser;
	IBOutlet NSTextField *newFolderText;
	IBOutlet NSButton *newFolderButton;
}
- (void)loadFiles;
- (NJBTransactionResult *)downloadFileList;
- (void)copyDataFileFromNJB:(DataFile *)dataFile;
- (void)copyDataFileToNJB:(MyItem *)dataFile;
- (IBAction)browserSingleClick:(id)sender;
- (IBAction)newFolder:(id)sender;
- (void)reloadNJBDirs;
- (void)deleteFromNJB:(MyItem *)item fromParent:(Directory *)parentDir;
- (BOOL)canCreateFolder;

@end
