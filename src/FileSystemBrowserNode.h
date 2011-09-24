//
//  FSNodeInfo.h
//
//  FSNodeInfo encapsulates information about a file or directory.
//  This implementation is not necessarily the best way to do something like this,
//  it is simply a wrapper to make the rest of the browser code easy to follow.

#import <Cocoa/Cocoa.h>

@interface FileSystemBrowserNode : NSObject {
@private
  NSString *relativePath;  // Path relative to the parent.
	FileSystemBrowserNode *parentNode;	// Containing directory, not retained to avoid retain/release cycles.
}

+ (FileSystemBrowserNode *)nodeWithParent:(FileSystemBrowserNode *)parent atRelativePath:(NSString *)path;

- (id)initWithParent:(FileSystemBrowserNode *)parent atRelativePath:(NSString*)path;

- (void)dealloc;

- (NSArray *)subNodes;
- (NSArray *)visibleSubNodes;

//- (NSString *)fsType;
- (NSString *)absolutePath;
- (NSString *)lastPathComponent;

- (BOOL)isLink;
- (BOOL)isDirectory;

- (BOOL)isReadable;
- (BOOL)isVisible;

- (NSImage*)iconImageOfSize:(NSSize)size; 

- (int)rowIndexForNode:(NSString *)nodeName;

@end
