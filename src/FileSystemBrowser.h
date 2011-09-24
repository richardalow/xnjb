//
//  FileSystemBrowser.h
//  XNJB
//
//  Created by Richard Low on Tue Jul 20 2004.
//

#import <Foundation/Foundation.h>
#import "FileSystemBrowserNode.h"

@interface FileSystemBrowser : NSObject {
@private
	NSBrowser *browser;
	NSString *baseLocation;
	id clickTarget;
	SEL clickFileAction;
	SEL clickDirectoryAction;
}
- (id)initWithBrowser:(NSBrowser *)newBrowser;
- (id)initWithBrowser:(NSBrowser *)newBrowser atLocation:(NSString *)newLocation;
- (void)setTarget:(id)newTarget;
- (void)setFileAction:(SEL)newFileAction;
- (void)setDirectoryAction:(SEL)newDirectoryAction;
- (void)setBrowser:(NSBrowser *)newBrowser;
- (NSBrowser *)browser;
- (void)setBaseLocation:(NSString *)newBaseLocation;
- (NSString *)baseLocation;
- (void)reloadData;
- (NSString *)path;
- (BOOL)directorySelected;
- (NSArray *)selectedFilesWithRecursion:(BOOL)recursion;
- (void)addContentsOfNode:(FileSystemBrowserNode *)node toArray:(NSMutableArray *)array;
- (NSString *)directory;
- (BOOL)isDirectory;
- (BOOL)isDirectory:(NSString *)filename;

- (IBAction)browserClick:(id)browser;
@end
