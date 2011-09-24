//
//  DragDropTableView.h
//  XNJB
//
//  Created by Richard Low on 13/12/2004.
//

#import <Cocoa/Cocoa.h>
#import "FileSystemBrowser.h"

@interface DragDropTableView : NSTableView {
	FileSystemBrowser *fsBrowser;
	BOOL allowCopies;
}
- (void)registerDragAndDrop;
- (void)disallowCopies;
- (void)allowCopies;

@end
